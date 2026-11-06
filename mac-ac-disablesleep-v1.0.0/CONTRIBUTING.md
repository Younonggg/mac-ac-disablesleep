# Contributing

Contributions are welcome.

## Development setup

Install Apple Command Line Tools, then run:

```bash
make check
```

This validates the LaunchDaemon plist and shell scripts, builds the native
helper, performs a dry run, and tests assertion creation.

## Pull requests

- Keep privileged code small and avoid invoking a shell from the daemon.
- Do not add credentials, telemetry, network listeners, or automatic downloads.
- Preserve the fail-safe battery behavior: any non-AC source maps to
  `disablesleep 0`.
- Describe physical testing across open-lid, closed-lid, AC-to-battery, and
  battery-to-AC transitions.
- Update `CHANGELOG.md` for user-visible changes.
