#!/bin/zsh
# SPDX-License-Identifier: MIT
set -u

label="com.local.disablesleep-by-power-source"

print "=== Current power source ==="
/usr/bin/pmset -g batt

print "\n=== disablesleep ==="
/usr/bin/pmset -g

print "\n=== Service ==="
if /bin/launchctl print "system/${label}" 2>/dev/null | /usr/bin/grep -E 'state =|pid =|runs =|last exit code'; then
    :
else
    print "Service is not loaded."
fi

print "\n=== Relevant power assertions ==="
/usr/bin/pmset -g assertions | /usr/bin/grep -E 'PreventSystemSleep|NetworkClientActive|Promote AC|Keep the Mac|Keep remote' || true
