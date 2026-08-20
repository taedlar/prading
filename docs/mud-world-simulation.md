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
| `World` | Owns the running world's shared state and its named zones. | A process-wide `std::shared_ptr<World>` instance. |
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
process-wide `World::instance`
              |
              v
       +-------------+       zones_: name -> shared Zone
       |    World    |----------------------------------+
       +-------------+                                  |
                                                    +---v---+
                                                    | Zone  |
                                                    +---^---+
                                                        |
                         current_zone_                  |
       +-------------+----------------------------------+
       |   Player    |
       | slot, entry |
       +------^------+
              |
   `Player::transports`: slot -> shared Player
              |
       mudmux connection
```

### `World`

`World` is the root of the simulation. It owns the `zones_` map, which maps a
stable zone name to a `std::shared_ptr<Zone>`. The running server creates the
single `World::instance` before calling `mudmux_run()` and destroys it after
the event loop returns. The same pointer is passed to `mudmux_run()` as the
hook context.

`World::CosmosType` is an alias that lets a project replace the concrete world
class without rewriting the application startup sequence. A specialized world
can add zone loading, persistence, scheduling, global rules, or registries for
other simulation objects.

The next API additions should make zone ownership explicit, for example with
lookup and registration operations. Zone names should be stable identifiers,
not player-facing display names.

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

`Player::transports` maps active transport slots to `std::shared_ptr<Player>`
instances. `Player::LogonType` is an alias for the concrete session type used
when a connection is created. A project may replace it with a login-session
class, then later create or attach a separate persistent character after
authentication.

Keeping the session and character distinct is recommended for reconnects,
multiple-character accounts, and administrative sessions, but it is not
required for the first implementation.

## Connection lifecycle

The transport layer is provided by `mudmux`; the simulation integrates through
its hooks. The intended lifecycle is:

1. `HOOK_CONNECT` creates a `Player::LogonType` for the slot and inserts it
   into `Player::transports`. This hook may also select transport protocols.
2. `HOOK_TRANSPORT_READY` begins the protocol-neutral login or character
   creation flow. It runs before the first inbound application message.
3. `HOOK_MESSAGE_INBOUND` (to be registered) looks up the player by slot,
   parses and dispatches the command, and applies authorized changes to the
   player and world.
4. `HOOK_DISCONNECT` saves or detaches any required session state and removes
   the slot-to-player mapping. It must tolerate a connection that did not
   complete login.
5. Server shutdown disconnects live slots before `World::instance` is reset,
   allowing session cleanup to access the world while it still exists.

The current source implements player creation in step 1 and establishes the
hook point for step 2, as well as the world startup/shutdown boundary. Login,
command dispatch, persistence, and disconnect cleanup remain implementation
work.

## Ownership and concurrency

`World::instance` and `Player::transports` are shared state. The current
classes provide `World::world_mutex` and `Player::transports_mutex` to guard
them, and each player has its own `mutex_` for player-local state.

`mudmux` can dispatch hooks through a worker pool, so world-simulation code
must not assume that callbacks for different slots are serialized. The
following locking discipline is proposed:

1. Acquire `World::world_mutex` before reading or mutating world-wide state or
   the zone map.
2. Acquire `Player::transports_mutex` only to find, insert, or remove a
   slot-to-player association; retain the returned `shared_ptr` before
   releasing the map lock.
3. Acquire a player's `mutex_` before changing its session-local state,
   including its current zone.
4. When an operation requires more than one lock, define and consistently use
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

1. Add safe world and player accessors, including zone lookup and an atomic
   player-movement operation.
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
