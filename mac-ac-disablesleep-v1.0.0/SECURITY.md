# Security policy

## Supported versions

Security fixes are applied to the latest release.

## Reporting a vulnerability

Please use GitHub's private vulnerability reporting feature from the
repository's **Security** tab. Do not include administrator passwords, device
passwords, private logs, serial numbers, or other credentials in an issue.

## Privileged components

The installer writes exactly these root-owned files:

- `/Library/PrivilegedHelperTools/com.local.disablesleep-by-power-source`
- `/Library/LaunchDaemons/com.local.disablesleep-by-power-source.plist`

The daemon accepts no network connections and executes `/usr/bin/pmset` with
fixed arguments selected from the current IOKit power-source state.
