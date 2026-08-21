# Agent Instructions for Library Build Units

`/lib` contains reusable library build units integrated into `prading`. A
library here may contain project code or third-party source, such as a vendored
open-source package; this directory does not establish code ownership or
licensing. Keep each third-party package's required license text, notices, and
provenance with the package. Keep driver startup, transport-hook registration,
and executable-specific behavior in `/src`.

## Library Design

- Give each library a focused responsibility and a dedicated CMake target.
- Expose its supported interface through public headers; keep implementation
  details private to the library directory.
- Depend on Git submodules only through their documented public headers and
  exported CMake targets. Do not copy submodule code or include private
  submodule headers.
- Avoid global mutable state. If shared state is necessary, document its owner,
  lifetime, and synchronization requirements.

## Build and Test

- Add library subdirectories after required submodules in the top-level
  `CMakeLists.txt`.
- Use target-based CMake dependencies (`target_link_libraries`) and propagate
  include directories and compile requirements through targets.
- Add unit tests for library behavior under `/tests`; do not make the driver
  executable a prerequisite for testing a generic library.
