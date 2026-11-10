# mac-ac-disablesleep

Power-aware closed-display sleep control for macOS laptops.

[简体中文](README.zh-CN.md)

`mac-ac-disablesleep` keeps a Mac awake and network-reachable while connected
to AC power, including after AC is connected while the lid is already closed.
On battery power, it restores the normal macOS closed-lid sleep behavior.

## Behavior

| Power source | `disablesleep` | Extra assertions | Closed-lid behavior |
| --- | ---: | --- | --- |
| AC power | `1` | Prevent system sleep and keep network clients active | Stays awake and reachable |
| Battery | `0` | Released | Uses the Mac's existing sleep settings |

The project does not change the existing battery sleep timer.

## Why a daemon is needed

`disablesleep` is a system-wide power setting. Static `pmset -b` and `pmset -c`
profiles cannot reliably maintain different `disablesleep` values. This project
uses a small root LaunchDaemon to listen for IOKit power-source notifications
and execute one of these fixed commands:

```text
/usr/bin/pmset -a disablesleep 1  # AC power
/usr/bin/pmset -a disablesleep 0  # battery or any non-AC source
```

On AC connection it also:

- holds `PreventSystemSleep` and `NetworkClientActive` assertions;
- holds a 30-second `UserIsActive` assertion to promote a dark wake to a full
  wake so Wi-Fi and remote-control software can reconnect;
- enables `acwake 1`, allowing a sleeping closed-lid Mac to react to a power
  source change.

The daemon reads the effective `SleepDisabled` value from `IOPMrootDomain`
after startup and every 15 seconds. It invokes `pmset` only when that value no
longer matches the current power source. This self-corrects macOS startup races
or later settings resets without repeatedly writing an already-correct value.

## Requirements

- A macOS laptop.
- An administrator account.
- Apple Command Line Tools.

The current release was physically tested on an Apple Silicon Mac. The
installer compiles the helper natively on the target Mac.

## Install

Install Command Line Tools if necessary:

```bash
xcode-select --install
```

Clone or download this repository, then run:

```bash
./scripts/install.sh
```

The installer requests administrator access through the standard `sudo`
prompt. It never stores the password. No reboot is required.

## Verify

```bash
./scripts/status.sh
```

While on AC power, the output should include:

```text
SleepDisabled  1
PreventSystemSleep  1
NetworkClientActive  1
```

For a physical closed-lid test:

1. Disconnect AC and close the lid.
2. Confirm the Mac goes offline.
3. Connect AC without opening the lid.
4. Allow 15–30 seconds for Wi-Fi and remote services to reconnect.

## Uninstall

```bash
./scripts/uninstall.sh
```

This stops the daemon, removes its two installed files, and restores
`disablesleep 0`. It leaves the current `acwake` value unchanged to avoid
overwriting a pre-existing preference. To disable `acwake` too:

```bash
./scripts/uninstall.sh --disable-acwake
```

## Safety and limitations

- A closed Mac that stays awake can generate significant heat. Do not place it
  in a bag, drawer, or other poorly ventilated space while connected to AC.
- Remote-control software must still be configured to launch automatically and
  have the required background and network permissions.
- Other sleep-management applications may override `pmset` or create
  conflicting assertions.
- A fully powered-off Mac cannot run the daemon or respond to power changes.
- `disablesleep` is not documented in the public `pmset` manual and may change
  in a future macOS release.

## License

[MIT](LICENSE)
