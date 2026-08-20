# prading

An experimental, **generic MUD foundation** for rapidly developing and extending MUDs with AI coding agents. The name `prading` means "starting" in Taiwan's Indigenous languages.

To start a new MUD project with `prading`, fork this repository.

## Bootstrap

The project uses Git submodules to organize its components. Fetch them after checking out the source tree:
```
git submodule update --init
```

If you fork this repository for a new MUD project, add modules under `/modules`.

## Building

Use `cmake --list-presets` to view the supported build configurations.

Configure and build the project with CMake. For example:
```sh
# Configure `linux-gcc` on Linux or WSL with the GNU C++ compiler.
cmake --preset linux-gcc

# Build the configured preset at out/build/<preset-name>.
cmake --build out/build/linux-gcc
```

## Development

Although `prading` includes C++ code and CMake build scripts, its primary development focus is the **agent instructions**.
Search the source tree with string `# Agent Instructions for` to locate these instruction files.

[`AGENTS.md`](https://agents.md/) files appear throughout the source tree and its submodules.
Together with the human-facing README files, these instructions are intended to make the repository friendly to coding agents and help MUD developers **code in natural language**.

The MUD driver in `/src` is deliberately kept small and generic on the `main` branch, providing a clean starting point for new MUD forks.

## Components

A MUD system consists of several essential parts:
- **Transport layer** — Accepts user connections; handles inbound and outbound data; and implements transport protocols such as TLS, Telnet, and WebSocket.
- **Logic layer** — Routes inbound data to command handlers and delivers outbound messages to connected users. It also defines or simulates the virtual world in which users interact. This layer has evolved through many architectures and design philosophies, including AberMUD, TinyMUD, LPMud, DikuMUD, MUSH, and MOO.
- **Content layer** — Contains the player-facing parts of the MUD. It can support many styles and themes: hack-and-slash MUDs present text-based worlds where players fight monsters or one another; role-playing MUDs encourage players to inhabit their characters; and social MUDs focus primarily on social interaction.

This three-layer component model abstracts a MUD system from a software developer's perspective.
`prading` aims to provide a generic foundation that MUD developers can use to assemble reusable submodules into a rapid-development codebase.
By having coding agents handle the lower layers (that is, transport and logic), MUD developers can focus on content, which benefits more from human creativity than coding skill.

## Hook Functions

Components are usually implemented as submodules in the `prading` source tree.
These submodules are typically assembled with hook functions: C functions that implement contracts defined by their components.

For example, when the system receives a user command, a transport-layer component processes it and logic-layer components route it to a command handler. The handler generates messages from content-layer descriptions, then sends them to users in the same room, as defined by the logic layer, through the transport-layer API.

A MUD with *in-game programming* capabilities, such as LPMud, may wire transport-layer hooks to interpreter-based callbacks. Other MUD genres, such as DikuMUD, may implement command routing with C command tables.

The logic layer determines how hook functions connect submodules.

## Submodules

As a generic MUD foundation, `prading` divides a MUD system into reusable submodules and encourages alternative implementations.

When adding a submodule, use `modules/mudmux` as an example and:
- Expose APIs with public C headers.
- Expose integration hook functions through a C ABI.
- Document contracts in agent instructions and human-readable documentation.

The following sections describe example submodules.

### User Persistence

When a client connects to the MUD server, it commonly goes through login or character creation before entering the world.
This is usually handled by a user-account subsystem that persists identities and credentials, such as passwords.

The user-account subsystem can be an independent logic-layer component or implemented directly in an in-game scripting language, such as LPC in LPMud, when suitable data storage is available.

### World Simulation

When a user enters the MUD, the system presents a virtual world for their *player character* to interact with.

MUDs with in-game programming capabilities typically define the virtual world with "objects" in a *mudlib*.
This gives developers considerable flexibility to create varied elements of the virtual world.
However, it can blur the boundary between the logic and content layers, resulting in inconsistency across the world unless the mudlib is carefully planned and organized.

Other MUD genres, such as DikuMUD, separate logic from content by using definition files.
The *driver*, written in C, loads these files to render virtual-world content through fixed logic.

Modern commercial online game systems, such as MMORPGs, commonly use a **hybrid design**.
For example, *zones* or *areas* may use 3D maps whose definition files are shared by graphical clients and the server, alongside a *script-based quest system* that lets content editors create varied quests without recompiling the server.

How the world simulation is divided into submodules and architectures is a design decision.
`prading` proposes a minimum viable [**world-simulation architecture**](docs/mud-world-simulation.md) built around three C++ classes: `World`, `Zone`, and `Player`.

> [!NOTE]
> This simulation model is independent of the choice to use, or not use, an in-game programming architecture such as LPMud.

### Content Management

The content layer is usually the soul of a MUD and the reason people love it.
Content creation often continues for years throughout a MUD's life and requires a range of support from the system.

In addition to game content, the content layer includes *tools* such as combat-system calculators, quest editors, and character-build simulators.

In MUDs with in-game programming, content-management tools are usually restricted to *wizards* or *immortals*.
Modern MMORPG servers also provide GM (*game master*) commands for managing game content online.

## Testing

The project uses GoogleTest as its unit-testing framework. Use `ctest` to run the unit tests:
```sh
# Run tests for the GCC build on Linux.
ctest --test-dir out/build/linux-gcc
```

## License

The `prading` codebase in this repository is released under the [MIT License](LICENSE).
Each Git submodule may have its own license terms, which apply to that submodule's repository.

## Credits

- Project owner and initial architecture: @taedlar
