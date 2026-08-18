# Agent Instructions for Using Git Submodules

All subdirectories in `/modules` are independent version-controlled repositories of re-usable Git submodules.

- Read agent instructions and documentations in the submodule directory before design or toubleshooting.
- A submoudle can provide multiple integration interfaces. Read integration notes below for the project's design decisions.
- **IMPORTANT**: Do not modify or duplicate submodule code. Treat submodule API contracts as source of truth and read-only. If implementation of a feature is blocked by API contract's limitation, ask for user decision before trying to bypass it. 

## Integration Notes

- `/mud.conf` is the global configuration file in YAML format.
- `/modules/mudmux` is used as the transport layer framework. Links to `mudmux::mudmux` as shared library.
