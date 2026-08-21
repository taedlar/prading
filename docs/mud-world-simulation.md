# MUD World Simulation

> [!WARNING]
> This document is a design draft. It describes the intended direction of the
> world-simulation layer; it is not yet a stable API or file-format
> specification.

## Purpose

This proposal gives `prading` a small, explicit model for the part of a MUD
that represents the persistent game world and the players currently connected
to it. It is deliberately minimal so that different MUD genres can extend it
without being forced into a particular content format, scripting language, or
gameplay model.

The initial model has three core concepts:

| Concept | Responsibility | Initial representation |
| --- | --- | --- |
| `World` | Defines the common world-simulation interface and shared state. | A polymorphic C++ base class. |
| `World::CosmosType` | Selects the concrete world implementation for this driver. | A process-wide `std::unique_ptr<CosmosType>` instance. |
| `Zone` | Represents an independently managed area of the world. | A polymorphic C++ base class. |
| `Player` | Represents a connected player's session and current location. | A polymorphic C++ base class associated with a transport slot. |

This is a logic-layer model. Content such as room descriptions, NPC templates,
quests, maps, and dialogue may be supplied by definition files, C++ code, an
in-game language, or another subsystem.

## Goals

- Establish a clear owner for shared world state.
- Keep transport concerns separate from game state while allowing a connection
  to be associated with a player session.
- Allow zones and players to be specialized by a game project.
- Support an incremental path from the current connection lifecycle to command
  dispatch, persistence, and richer simulations.
- Make synchronization responsibilities explicit when transport callbacks run
  concurrently.

## Non-goals

This proposal does not prescribe:

- a room, exit, item, combat, NPC, quest, or economy model;
- a persistence backend or a world-definition format;
- a command parser or permission system;
- a scripting runtime; or
- a distributed, multi-process simulation.

Those concerns can be introduced as separate submodules or project-specific
extensions once their contracts are known.

## Object model

```text
World lifecycle API --> unique_ptr<CosmosType>
                          |
                          v
                   CosmosType -- derives from --> World
                                                  |
                                                  | private zones_: name -> shared Zone
                                                  v
                                                 Zone
                                                  ^
                                                  | private current_zone_
                                                Player
                                                  ^
                                                  | private transports_: slot -> shared Player
                                                  |
                                           mudmux connection
```

### `World`

`World` is the polymorphic base class for world simulations. It defines the
common world state, including the `zones_` map from stable zone names to
`std::shared_ptr<Zone>` instances.

`World::CosmosType` identifies the concrete world type used by this driver. It
defaults to `World`, but a project can set the alias to a class derived from
`World`. The private `std::unique_ptr<CosmosType>` is managed through
`World::initialize()`, `World::get_instance()`, and `World::shutdown()`: the
running server creates that sole owner before calling `mudmux_run()`. The
pointer passed to `mudmux_run()` is a non-owning hook context, not a second
owner. Zones are accessed through `set_zone()`, `get_zone()`, and
`remove_zone()`.

### World lifetime contract

On a normal shutdown, the driver must preserve this ordering:

1. `mudmux_run(World::get_instance())` returns only after the transport event
   loop has stopped.
2. Call `World::shutdown()` to run the concrete `CosmosType` destructor.
3. Call `mudmux_deinit()` only after `World::shutdown()` has completed.

Using `std::unique_ptr<CosmosType>` makes this destruction point deterministic:
no copied shared owner can keep the concrete world alive past `mudmux_deinit()`.
This is important in a multithreaded driver, where a late world destructor
could otherwise depend on transport-layer resources that have already been
released.

Hooks and worker tasks may use the non-owning context only while `mudmux_run()`
is active. They must not store the context pointer, create another owner for
the world, or access it after the event loop returns.

A specialized `CosmosType` can add zone loading, persistence, scheduling,
global rules, or registries for other simulation objects without rewriting the
application startup sequence.

Zone names should be stable identifiers, not player-facing display names.

### `Zone`

`Zone` is an independently managed portion of the world. Depending on the
game, it may represent a geographical region, a dungeon instance, a social
space, a shard, or another useful unit of simulation and content loading.

The base class intentionally contains no room or content schema. A derived
class can load static definitions, create dynamic state, expose movement and
visibility operations, or delegate behavior to a scripting runtime. This lets
one project use data-driven DikuMUD-style areas while another uses objects from
an in-game language.

### `Player`

`Player` represents a live session, not necessarily a persistent character.
Initially it stores:

- the mudmux transport `slot_` associated with the session;
- `entry_name_`, the configured transport entry supplied at connection time;
- `current_zone_`, the zone containing the player; and
- a per-player mutex for state that belongs exclusively to that player.

The private transport map maps active transport slots to
`std::shared_ptr<Player>` instances and is accessed through
`Player::connect()`, `Player::find_by_slot()`, and `Player::disconnect()`. Removal is
conditional on the expected `Player`, so a delayed disconnect cannot erase a
new session that reuses the same slot.
`Player::LogonType` is an alias for the concrete session type used when a
connection is created. A project may replace it with a login-session class,
then later create or attach a separate persistent character after
authentication.

Keeping the session and character distinct is recommended for reconnects,
multiple-character accounts, and administrative sessions, but it is not
required for the first implementation.

## Connection lifecycle

The transport layer is provided by `mudmux`; the simulation integrates through
its hooks. The intended lifecycle is:

1. `HOOK_CONNECT` calls `Player::connect()` to connect a
   `Player::LogonType` for the slot. This hook may also select transport
   protocols.
2. `HOOK_TRANSPORT_READY` begins the protocol-neutral login or character
   creation flow. It runs before the first inbound application message.
3. `HOOK_MESSAGE_INBOUND` (to be registered) looks up the player by slot,
   parses and dispatches the command, and applies authorized changes to the
   player and world.
4. `HOOK_DISCONNECT` saves or detaches any required session state and removes
   the slot-to-player mapping. It must tolerate a connection that did not
   complete login.
5. Server shutdown disconnects live slots before `mudmux_run()` returns,
   allowing session cleanup to access the world while it still exists. The
   driver then calls `World::shutdown()` before calling `mudmux_deinit()`.

The current source implements player creation in step 1, establishes the hook
point for step 2, and enforces the world lifetime contract. Login, command
dispatch, persistence, and disconnect cleanup remain implementation work.

## Ownership and concurrency

The world instance has a single owner of the concrete `CosmosType`, but the
world it manages is shared state while the event loop is running. The transport
map is also shared state. Those members and their mutexes are private; callers
use the thread-safe `World` and `Player` methods instead.

`mudmux` can dispatch hooks through a worker pool, so world-simulation code
must not assume that callbacks for different slots are serialized. The
following locking discipline is proposed:

1. Use `World`'s lifecycle and zone methods to read or mutate world-wide
   state. `get_instance()` is valid only during the event-loop lifetime.
2. Use `Player::connect()`, `Player::find_by_slot()`, or
   `Player::disconnect()` for slot-to-player associations. Retain the returned
   `shared_ptr` after the call when a session must outlive its map entry, and
   pass it to `Player::disconnect()` when cleanup completes.
3. Use a player's accessors to inspect its slot, entry name, or current zone,
   and `set_current_zone()` to change its session-local state.
4. When an operation requires more than one state operation, define and consistently use
   one lock order before implementation. Do not hold world or player locks
   while performing blocking I/O, persistence calls, or callbacks into an
   untrusted scripting runtime.

The existing recursive mutexes allow re-entrant code during early development,
but recursive locking is not a substitute for a documented lock order. A
future revision may replace them with narrower, non-recursive synchronization
once the API is established.

## Extension path

The architecture is intended to grow through contracts rather than by placing
all game rules in the transport callbacks:

1. Add an atomic player-movement operation.
2. Register an inbound-message hook and introduce a command-dispatch
   interface that receives a player and a command.
3. Define a zone-content interface for rooms, exits, and visible entities.
4. Add a persistence component for accounts, characters, and durable world
   state.
5. Add optional adapters for definition files, an in-game programming runtime,
   or editor tooling.

Each extension should specify its ownership, thread-safety, lifecycle, and
failure behavior. The `World`/`Zone`/`Player` model remains a stable boundary
between transport sessions and the game-specific simulation.

## Open questions

- Should a `Player` remain a session object, with `Character` introduced as a
  separate persistent entity from the outset?
- Should zones be loaded eagerly at startup, lazily on first entry, or managed
  by a dedicated zone service?
- Which operations need to be transactional, particularly movement across
  zones and persistence during disconnect?
- Should the initial command dispatcher be a C++ interface, a hook contract,
  or an adapter around an embedded scripting runtime?
- What identifiers must remain stable across content revisions and persisted
  save data?
