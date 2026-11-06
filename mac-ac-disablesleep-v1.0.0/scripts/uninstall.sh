#!/bin/zsh
# SPDX-License-Identifier: MIT
set -euo pipefail

label="com.local.disablesleep-by-power-source"
helper_path="/Library/PrivilegedHelperTools/${label}"
plist_path="/Library/LaunchDaemons/${label}.plist"

if [[ "${1:-}" != "" && "${1:-}" != "--disable-acwake" ]]; then
    print -u2 "Usage: $0 [--disable-acwake]"
    exit 1
fi

print "Administrator access is required to uninstall the system service."
sudo -v

if sudo /bin/launchctl print "system/${label}" >/dev/null 2>&1; then
    sudo /bin/launchctl bootout "system/${label}"
fi

sudo /usr/bin/pmset -a disablesleep 0

if [[ "${1:-}" == "--disable-acwake" ]]; then
    sudo /usr/bin/pmset -a acwake 0
fi

sudo /bin/rm -f "${helper_path}" "${plist_path}"

print "Uninstall complete. disablesleep has been restored to 0."
