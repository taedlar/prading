# Agent Instructions for the MUD Driver

`/src` contains the `prading` executable: its startup and shutdown sequence,
transport-layer hook registration, and wiring between reusable components. Keep
the driver small, generic, and suitable as a starting point for downstream MUD
projects.

## Responsibilities

- Configure and start the transport layer, then construct and pass the
  application context required by its hooks.
- Register transport hooks and route them to project-owned logic or libraries.
- Read the global `mud.conf` configuration and pass each component only the
  configuration contract it supports.
- Keep genre-specific content, world definitions, and gameplay rules out of
  the driver unless they are essential to the generic foundation.

## Dependencies and Boundaries

- Use reusable project-owned logic from `/lib` through its public interface.
- Integrate `mudmux` only through its public headers and the
  `mudmux::mudmux` CMake target. Read `modules/AGENTS.md` and the `mudmux`
  documentation before changing hook integration.
- Do not modify source files inside `/modules` to implement driver behavior.
  If the documented submodule API cannot support the change, stop and request
  direction on a submodule change.

## Verification

- Update or add tests under `/tests` for driver behavior that can be tested
  outside a transport integration run.
- Configure and build using a supported CMake preset, then run the relevant
  `ctest` suite for changes that affect executable behavior.
