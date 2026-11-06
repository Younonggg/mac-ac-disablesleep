# Changelog

All notable changes to this project are documented here. This project follows
[Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-11-06

### Added

- Event-driven IOKit power-source monitoring.
- Conditional `pmset -a disablesleep 1/0` switching.
- AC-only system-sleep and network-client assertions.
- A 30-second user-active assertion to promote AC connection from dark wake.
- `acwake` setup for closed-lid AC reconnection.
- Root LaunchDaemon installation, status, and uninstall scripts.
- English and Simplified Chinese documentation.
- macOS GitHub Actions build and validation workflow.
