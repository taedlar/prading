# prading

An experimental, **generic MUD foundation** designed for rapid development and extension with AI coding agents. The name `prading` means "starting" in Taiwan's Indigenous languages.

Start your new MUD project with `prading` by forking this repository.

## Bootstrap

The project uses Git submodules to organize multiple components.
Fetch the submodules after checking out the source tree:
```
git submodule update --init
```

If you fork this repository to start a new MUD project, add new modules under `/modules`.

## Building

Use `cmake --list-presets` to view the supported configurations.

Configure and build the project with CMake. For example:
```sh
# configure `linux-gcc` (on Linux or WSL, using GNU C++ compiler)
cmake --preset linux-gcc

# build configured preset at out/build/<preset-name>
cmake --build out/build/linux-gcc
```

## Components

A MUD system consists of several essential parts:
- **Transport layer** — Accepts user connections; handles inbound and outbound data; and implements transport protocols such as TLS, Telnet, and WebSocket.
- **Logic layer** — Routes inbound data to command handlers and delivers outbound messages to connected users. It defines or simulates the virtual world in which users interact. This layer has evolved through many architectures and design philosophies, including AberMUD, TinyMUD, LPMud, DikuMUD, MUSH, and MOO.
- **Content layer** — Contains the player-facing parts of the MUD. It can support many styles and themes: hack-and-slash MUDs present text-based worlds where players fight monsters or one another; role-playing MUDs encourage players to inhabit their characters; and social MUDs focus primarily on social interaction.

`prading` aims to provide a generic foundation that MUD developers can use to assemble reusable submodules into a rapid-development codebase, with help from AI coding agents.
