# Agent Instructions for the `prading` Repository

## Source Tree Structure

- `/modules`: independent Git submodule repositories
- `/src`: the MUD driver executable
- `/lib`: library build units integrated into this repository
- `/docs`: project documentation
- `/tests`: unit tests for project-owned code
- `/examples`: example code for both human developer and coding agent

`/modules` contains independent repositories. Each submodule can have its own
build, tests, licensing, and `AGENTS.md` instructions. `/src`, `/lib`,
`/docs`, and `/tests` belong to this repository and follow this file and any
more-specific instructions within those directories. `/lib` is a build-layout
category, not an ownership claim: it may contain project code or third-party
source integrated as a CMake library target. Preserve the provenance, license,
and notices for third-party source. Do not apply a submodule's conventions to
project-owned code, or project-owned conventions to a submodule.

## Working with Git Submodules

- Before designing against a submodule, read its README, public headers, and
  applicable `AGENTS.md` files. Treat its documented public API as the
  integration contract.
- Keep project-specific code in `/src`, `/lib`, `/docs`, or `/tests`; do not
  copy or duplicate submodule implementation code into this repository.
- Do not modify files inside `/modules/<name>` unless the task explicitly
  requires a change to that submodule. Such a change belongs to the
  submodule's repository and must follow its local instructions.
- Update the Gitlink recorded by this repository only when intentionally
  advancing the submodule revision. Review the parent repository's Gitlink
  change separately from changes inside the submodule.
- When a needed submodule API is missing, propose an upstream submodule change
  or ask for direction; do not work around the contract by reaching into
  private headers or implementation files.

## CMake Dependencies

- Add required submodules with `add_subdirectory(modules/<name>)` before
  libraries from `/lib` and the driver.
- If a submodule makes a dependency available to the parent build, prefer
  `find_package(<dependency> REQUIRED)` and its exported target over fetching
  or defining the dependency again.
- Add libraries from `/lib` after submodules, then add `/src` and `/tests`.
  Link CMake targets by their exported target names rather than by raw include
  or library paths.
- For project-wide configuration settings and `spdlog` header, always add
  `target_include_directories(<target> PRIVATE ${CMAKE_BINARY_DIR})` to the
  CMakeList.txt and add `#include "config.h"` at begining of C/C++ implementation
  files. Also add `target_link_libraries(<target>, PRIVATE spdlog::spdlog)` to
  link with the spdlog library.
