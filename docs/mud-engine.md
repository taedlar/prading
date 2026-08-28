# MUD Engine

> [!WARNING]
> This document describes the current engine boundary and its intended
> extension path. It is not yet a stable framework API for plugins, scripting
> runtimes, or game rules.

## Purpose

In `prading`, the **engine** is the driver-owned runtime that coordinates
long-lived services needed to run a MUD. It is distinct from both the
transport event loop and the world simulation:

- `mudmux` owns connections, protocols, and the asynchronous event loop.
- `Engine` owns engine-level runtime services, such as a command dispatcher,
  scheduler, scripting virtual machine, or shared registries.
- `World`, `Zone`, and `Player` represent game state and connected sessions.

The separation is deliberately small. It gives a downstream MUD a clear place
to introduce genre-specific runtime behavior without making the generic driver
itself responsible for a game's content or rules. The companion
[world-simulation design](mud-world-simulation.md) defines the initial model
for `World`, `Zone`, and `Player`.

## Goals

- Provide a well-defined lifetime for engine-level services.
- Keep transport concerns separate from game logic and content.
- Support compiled, in-game-programming, and hybrid MUD architectures.
- Let transport hooks delegate to explicit application services instead of
  accumulating game rules in callbacks.
- Make ownership and shutdown dependencies clear as the driver grows.

## Non-goals

The base `Engine` does not prescribe:

- a command syntax, dispatcher, permission system, or gameplay model;
- a room, item, combat, NPC, quest, or economy model;
- an embedded language, virtual machine, or mudlib interface;
- persistence, scheduling, or content-loading formats; or
- a replacement event loop or transport implementation.

Those are project-specific components or future contracts. They may live in a
fork, in a library under `/lib`, or in a reusable submodule when they have a
documented public API.

## Current model

`Engine` is a polymorphic C++ base class constructed by the driver. Its
constructor and virtual destructor establish the lifetime of engine-owned
resources:

| Operation | Responsibility |
| --- | --- |
| `Engine` construction | Initializes engine-level runtime services before `World` is constructed. |
| `Engine` destruction | Releases those services after `World` has been destroyed. |

A downstream project constructs its derived engine explicitly. `Engine` is not
a process-global singleton; code that needs an engine dependency receives it
through construction or a narrowly scoped interface.

## Lifecycle and ownership

The driver initializes its core objects in this order:

```text
configure mudmux
       |
       v
Engine engine;
       |
       v
World world(engine) --> mudmux_run(&world)
                              |
                              v
                    transport hooks run
                              |
                              v
                       world destructor
                              |
                              v
                       engine destructor --> mudmux_deinit()
```

`mudmux_run()` receives a non-owning pointer to `World` as its hook context.
The `World` constructor receives a non-owning `Engine&`; derived world classes
can use that protected dependency while the engine outlives the world.

The ordering is intentional:

1. The engine exists before the world, so world setup may use engine services.
2. Scope exit destroys the world before the engine, so engine-owned services
   remain available for world and session cleanup.
3. `mudmux_deinit()` runs last, after objects that may depend on transport
   resources have been destroyed.

Normal C++ scope unwinding preserves this order if construction fails. New
startup stages should preserve the same dependency ordering.

## Transport integration boundary

The driver registers `HOOK_CONNECT`, `HOOK_TRANSPORT_READY`, and
`HOOK_DISCONNECT` with `mudmux`. Their current responsibilities are minimal:

| Hook | Current responsibility | Typical engine extension |
| --- | --- | --- |
| `HOOK_CONNECT` | Create and associate a `Player` session with the transport slot. | Select protocol features or initialize session-facing services. |
| `HOOK_TRANSPORT_READY` | Establish a protocol-neutral point after transport setup. | Start login, character selection, or a welcome flow. |
| `HOOK_DISCONNECT` | Remove the active player session. | Save, detach, or notify engine-managed systems. |

An inbound-message hook can later look up the `Player`, parse a command, and
delegate it to an engine-owned command service. Hooks should remain adapters:
they translate transport events into calls on application services, rather
than becoming the primary location for gameplay rules or content definitions.

`mudmux` may invoke hooks concurrently. Engine services that share mutable
state must define their own synchronization rules. Do not hold an engine or
world lock while performing blocking I/O, persistence operations, or callbacks
into an untrusted scripting runtime.

## Architectural styles

The engine/world boundary supports several familiar MUD designs.

### Compiled-world (DikuMUD-style)

In a DikuMUD-style architecture, the engine commonly owns component
initialization, command dispatch, global registries, and runtime resources.
Game rules operate directly on C++ world state, often with data-driven area or
definition files. An engine may also host limited scripting for quests, NPCs,
or other subsystems without making scripts the primary definition of the
world.

### In-game-programming (LPMud-style)

In an LPMud-style architecture, the engine hosts an interpreter and virtual
machine. Transport hooks are adapted to calls on script-defined objects in the
*mudlib*. The engine manages virtual-machine and object lifecycles and exposes
only the host APIs needed by scripts to implement logic and content.

The script runtime is an engine service; it does not remove the need to define
ownership, cleanup, concurrency, and error boundaries between C++ and script
code.

### Hybrid

Most systems combine these approaches. For example, a compiled engine may
load zones from definition files while delegating quests to scripts, or a
script-defined mudlib may call compiled services for persistence, pathfinding,
or protocol features. The important design choice is to document which side
owns each service and which interface crosses the boundary.

## Extension guidance

When adding an engine service, define its contract before wiring it into a
transport callback:

1. State what the service owns and which component is allowed to call it.
2. Define startup, normal shutdown, initialization-failure, and reload
   behavior.
3. Specify whether it may access `World`, `Zone`, or `Player`, and whether it
   stores non-owning references to them.
4. Define thread-safety, lock ordering, and whether callbacks may re-enter the
   service.
5. State how failures are reported and whether they disconnect a player, abort
   an operation, or stop the server.

Keep project-specific engines out of the generic driver where practical. A
small driver that wires public component interfaces is easier for a MUD fork to
replace than one that embeds a particular genre's rules.

## Open questions

- Which engine services belong in project-owned libraries, and which have a
  stable enough contract to become independent submodules?
- What command-dispatch interface best supports both C++ handlers and embedded
  runtimes?
- How should reloadable content and scripts synchronize with live player
  sessions and world state?
