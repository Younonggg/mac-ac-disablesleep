#!/bin/zsh
# SPDX-License-Identifier: MIT
set -euo pipefail

label="com.local.disablesleep-by-power-source"
helper_path="/Library/PrivilegedHelperTools/${label}"
plist_path="/Library/LaunchDaemons/${label}.plist"
repo_root="${0:A:h:h}"
source_path="${repo_root}/src/disablesleep_by_power_source.c"
plist_source="${repo_root}/launchd/${label}.plist"

if [[ "$(uname -s)" != "Darwin" ]]; then
    print -u2 "This installer only supports macOS."
    exit 1
fi

if ! clang_path="$(xcrun --find clang 2>/dev/null)"; then
    print -u2 "Apple Command Line Tools are required. Run: xcode-select --install"
    exit 1
fi

/usr/bin/plutil -lint "${plist_source}"

build_dir="$(mktemp -d "${TMPDIR:-/tmp}/${label}.XXXXXX")"
cleanup() {
    /bin/rm -f "${build_dir}/${label}"
    /bin/rmdir "${build_dir}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

print "Building the power-source daemon..."
"${clang_path}" \
    -Wall -Wextra -Werror -O2 \
    -framework CoreFoundation \
    -framework IOKit \
    "${source_path}" \
    -o "${build_dir}/${label}"

/usr/bin/codesign --force --sign - "${build_dir}/${label}"
"${build_dir}/${label}" --test-assertions

print "Administrator access is required to install the system service."
sudo -v
sudo /usr/bin/install -o root -g wheel -m 755 \
    "${build_dir}/${label}" "${helper_path}"
sudo /usr/bin/install -o root -g wheel -m 644 \
    "${plist_source}" "${plist_path}"
sudo /usr/bin/pmset -a acwake 1

if sudo /bin/launchctl print "system/${label}" >/dev/null 2>&1; then
    sudo /bin/launchctl kickstart -k "system/${label}"
else
    sudo /bin/launchctl bootstrap system "${plist_path}"
fi

print
print "Installation complete. No reboot is required."
/usr/bin/pmset -g batt
/usr/bin/pmset -g
sudo /bin/launchctl print "system/${label}" | /usr/bin/grep -E 'state =|pid =|last exit code' || true
