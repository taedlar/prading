# Agent Instructions for Git Submodules

Each direct child of `/modules` is an independent, reusable Git submodule
repository. The parent `prading` repository records only the commit selected
for each submodule; it does not own the submodule's source files.

## Rules for Integration

- Before using a submodule, read its `AGENTS.md`, README, and relevant public
  API documentation. Its own instructions govern work inside that repository.
- Use only documented public headers, exported CMake targets, and documented
  hook or callback contracts. Do not include private headers or depend on
  implementation details.
- Keep adapters and application policy in the parent repository. Do not copy
  submodule code into `/src` or `/lib`.
- Do not edit a submodule as part of parent-repository work unless explicitly
  requested. If an API limitation blocks integration, document the limitation
  and request a decision about changing the submodule.
- Treat a submodule revision update as a deliberate parent-repository change:
  verify the selected commit and review the Gitlink separately.

## Integration Notes

- `/mud.conf` is the driver's global YAML configuration file. Components may
  read the settings they own and ignore the rest; for example, its contents are
  passed to `mudmux_init()`.
- `modules/mudmux` supplies the transport-layer framework. Link the driver to
  the exported `mudmux::mudmux` target and use the public headers under
  `mudmux/`.
- Add `modules/mudmux` with `add_subdirectory(modules/mudmux)` before targets
  that consume it. Import dependencies it exposes with `find_package` where
  available instead of fetching a second copy.
