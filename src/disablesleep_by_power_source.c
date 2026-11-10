// SPDX-License-Identifier: MIT

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include <errno.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int last_applied_state = -1;
static bool dry_run = false;
static IOPMAssertionID prevent_sleep_assertion = kIOPMNullAssertionID;
static IOPMAssertionID network_client_assertion = kIOPMNullAssertionID;
static IOPMAssertionID user_active_assertion = kIOPMNullAssertionID;
static CFRunLoopTimerRef user_active_timer = NULL;

static void release_user_active_assertion(CFRunLoopTimerRef timer, void *context)
{
    (void)timer;
    (void)context;

    if (user_active_assertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(user_active_assertion);
        user_active_assertion = kIOPMNullAssertionID;
    }

    if (user_active_timer != NULL) {
        CFRelease(user_active_timer);
        user_active_timer = NULL;
    }
}

static bool enable_ac_wake_assertions(bool promote_user_active)
{
    if (prevent_sleep_assertion == kIOPMNullAssertionID) {
        IOReturn result = IOPMAssertionCreateWithName(
            kIOPMAssertionTypePreventSystemSleep,
            kIOPMAssertionLevelOn,
            CFSTR("Keep the Mac awake while connected to AC power"),
            &prevent_sleep_assertion);
        if (result != kIOReturnSuccess) {
            fprintf(stderr, "Unable to create AC prevent-sleep assertion: 0x%x.\n", result);
            return false;
        }
    }

    if (network_client_assertion == kIOPMNullAssertionID) {
        IOReturn result = IOPMAssertionCreateWithName(
            kIOPMAssertNetworkClientActive,
            kIOPMAssertionLevelOn,
            CFSTR("Keep remote network services available while on AC power"),
            &network_client_assertion);
        if (result != kIOReturnSuccess) {
            fprintf(stderr, "Unable to create network-client assertion: 0x%x.\n", result);
            return false;
        }
    }

    if (!promote_user_active) {
        return true;
    }

    if (user_active_timer != NULL) {
        CFRunLoopTimerInvalidate(user_active_timer);
        CFRelease(user_active_timer);
        user_active_timer = NULL;
    }
    if (user_active_assertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(user_active_assertion);
        user_active_assertion = kIOPMNullAssertionID;
    }

    IOReturn result = IOPMAssertionCreateWithName(
        CFSTR("UserIsActive"),
        kIOPMAssertionLevelOn,
        CFSTR("Promote AC connection from dark wake"),
        &user_active_assertion);
    if (result != kIOReturnSuccess) {
        fprintf(stderr, "Unable to create user-active assertion: 0x%x.\n", result);
        return false;
    }

    CFRunLoopTimerContext timer_context = {0, NULL, NULL, NULL, NULL};
    user_active_timer = CFRunLoopTimerCreate(
        kCFAllocatorDefault,
        CFAbsoluteTimeGetCurrent() + 30.0,
        0.0,
        0,
        0,
        release_user_active_assertion,
        &timer_context);
    if (user_active_timer == NULL) {
        fprintf(stderr, "Unable to create user-active assertion timer.\n");
        IOPMAssertionRelease(user_active_assertion);
        user_active_assertion = kIOPMNullAssertionID;
        return false;
    }
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), user_active_timer, kCFRunLoopDefaultMode);
    return true;
}

static void disable_ac_wake_assertions(void)
{
    if (user_active_timer != NULL) {
        CFRunLoopTimerInvalidate(user_active_timer);
        CFRelease(user_active_timer);
        user_active_timer = NULL;
    }
    if (user_active_assertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(user_active_assertion);
        user_active_assertion = kIOPMNullAssertionID;
    }
    if (prevent_sleep_assertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(prevent_sleep_assertion);
        prevent_sleep_assertion = kIOPMNullAssertionID;
    }
    if (network_client_assertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(network_client_assertion);
        network_client_assertion = kIOPMNullAssertionID;
    }
}

static int desired_disablesleep_state(void)
{
    CFTypeRef snapshot = IOPSCopyPowerSourcesInfo();
    if (snapshot == NULL) {
        return -1;
    }

    CFStringRef source = IOPSGetProvidingPowerSourceType(snapshot);
    int state = 0;
    if (source != NULL &&
        CFStringCompare(source, CFSTR(kIOPMACPowerKey), 0) == kCFCompareEqualTo) {
        state = 1;
    }

    CFRelease(snapshot);
    return state;
}

static int current_disablesleep_state(void)
{
    io_service_t root_domain = IOServiceGetMatchingService(
        kIOMainPortDefault,
        IOServiceMatching("IOPMrootDomain"));
    if (root_domain == IO_OBJECT_NULL) {
        return -1;
    }

    CFTypeRef value = IORegistryEntryCreateCFProperty(
        root_domain,
        CFSTR("SleepDisabled"),
        kCFAllocatorDefault,
        0);
    IOObjectRelease(root_domain);
    if (value == NULL) {
        return -1;
    }

    int state = -1;
    if (CFGetTypeID(value) == CFBooleanGetTypeID()) {
        state = CFBooleanGetValue((CFBooleanRef)value) ? 1 : 0;
    } else if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        int number = 0;
        if (CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &number)) {
            state = number != 0 ? 1 : 0;
        }
    }

    CFRelease(value);
    return state;
}

static bool apply_current_state(void)
{
    int state = desired_disablesleep_state();
    if (state < 0) {
        fprintf(stderr, "Unable to determine the current power source.\n");
        return false;
    }

    const char *value = state == 1 ? "1" : "0";
    if (dry_run) {
        printf("/usr/bin/pmset -a disablesleep %s\n", value);
        last_applied_state = state;
        return true;
    }

    int current_state = current_disablesleep_state();
    bool source_changed = state != last_applied_state;
    bool setting_mismatch = current_state != state;

    if (!source_changed && !setting_mismatch) {
        if (state == 1) {
            return enable_ac_wake_assertions(false);
        }
        return true;
    }

    if (state == 1) {
        if (!enable_ac_wake_assertions(source_changed)) {
            return false;
        }
    } else {
        disable_ac_wake_assertions();
    }

    if (!setting_mismatch) {
        last_applied_state = state;
        return true;
    }

    char *const arguments[] = {
        "/usr/bin/pmset",
        "-a",
        "disablesleep",
        (char *)value,
        NULL
    };

    pid_t child = 0;
    int spawn_result = posix_spawn(&child, arguments[0], NULL, NULL, arguments, environ);
    if (spawn_result != 0) {
        fprintf(stderr, "Unable to start pmset: %s\n", strerror(spawn_result));
        return false;
    }

    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return false;
        }
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "pmset failed with status %d.\n", status);
        return false;
    }

    last_applied_state = state;
    syslog(LOG_NOTICE,
           "Applied disablesleep=%d for %s power; AC wake assertions are %s.",
           state,
           state == 1 ? "AC" : "battery",
           state == 1 ? "enabled" : "disabled");
    return true;
}

static void power_source_changed(void *context)
{
    (void)context;
    if (!apply_current_state()) {
        exit(EXIT_FAILURE);
    }
}

static void reconcile_timer_fired(CFRunLoopTimerRef timer, void *context)
{
    (void)timer;
    (void)context;
    if (!apply_current_state()) {
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    openlog("disablesleep-by-power-source", LOG_PID, LOG_DAEMON);

    if (argc == 2 && strcmp(argv[1], "--dry-run") == 0) {
        dry_run = true;
        return apply_current_state() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (argc == 2 && strcmp(argv[1], "--test-assertions") == 0) {
        if (!enable_ac_wake_assertions(true)) {
            return EXIT_FAILURE;
        }
        printf("AC wake assertions created successfully.\n");
        disable_ac_wake_assertions();
        return EXIT_SUCCESS;
    }

    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--dry-run|--test-assertions]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (geteuid() != 0) {
        fprintf(stderr, "This service must run as root.\n");
        return EXIT_FAILURE;
    }

    if (!apply_current_state()) {
        return EXIT_FAILURE;
    }

    CFRunLoopSourceRef source =
        IOPSNotificationCreateRunLoopSource(power_source_changed, NULL);
    if (source == NULL) {
        fprintf(stderr, "Unable to register for power-source notifications.\n");
        return EXIT_FAILURE;
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);

    CFRunLoopTimerContext timer_context = {0, NULL, NULL, NULL, NULL};
    CFRunLoopTimerRef reconcile_timer = CFRunLoopTimerCreate(
        kCFAllocatorDefault,
        CFAbsoluteTimeGetCurrent() + 3.0,
        15.0,
        0,
        0,
        reconcile_timer_fired,
        &timer_context);
    if (reconcile_timer == NULL) {
        fprintf(stderr, "Unable to create the power-setting reconcile timer.\n");
        CFRelease(source);
        return EXIT_FAILURE;
    }
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), reconcile_timer, kCFRunLoopDefaultMode);

    CFRunLoopRun();
    CFRelease(reconcile_timer);
    CFRelease(source);
    return EXIT_SUCCESS;
}
