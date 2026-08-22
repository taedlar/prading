# Agent Instructions for the Player Store

The player store is a project-owned library build unit under `/lib`. Keep its
public API focused on durable player records backed by SQLite. Do not move
MUD-driver behavior or session management into this library.

## Responsibility

The library owns:

- Opening or creating a player-store database.
- Validating player identifiers at the API boundary.
- Creating and migrating the player-store schema.
- Atomically authenticating a player and recording a successful login.
- Replacing, restoring, and removing serialized player data.

The library does not own password input, password-hash generation, player
object construction, or JSON/other serialization logic. Callers provide a PHC
password hash and serialized player text; the library stores and returns those
values.

## Storage Contract

Use one table for the MVP:

```sql
CREATE TABLE IF NOT EXISTS players (
    username TEXT PRIMARY KEY,
    password_hash TEXT NOT NULL,
    last_login INTEGER,
    last_saved INTEGER NOT NULL,
    player_text TEXT NOT NULL,
    player_version INTEGER NOT NULL DEFAULT 1
);
```

- `username` is the stable primary key and must have a unique constraint.
- Timestamps are UTC Unix time in seconds. A null `last_login` means the
  player has never logged in successfully.
- `player_text` is opaque to this library and must be persisted without
  normalization or interpretation.
- `player_version` identifies the serialized player-data format. New writes
  use the current supported version; restores must expose the version so the
  caller can select compatible deserialization or migration logic.
- Schema initialization must be idempotent. Use an explicit schema-version
  mechanism when a future change cannot be represented by `CREATE TABLE IF
  NOT EXISTS`.

Do not store plaintext passwords, authentication tokens, or mutable in-memory
player objects in the database.

## API Semantics

- `open` returns an owned store handle or a structured error. It creates the
  database and schema when absent and applies a bounded SQLite busy timeout.
- `login` looks up the username, verifies the supplied password against the
  stored PHC hash, and updates `last_login` only after successful verification.
  The verification and timestamp update must be one logical operation. Failed
  credentials must not modify the row. Do not reveal whether a username exists
  to callers that only need an authentication result.
- `restore` returns a value containing the stored metadata and serialized
  player text. It must not expose SQLite statements, rows, or a mutable
  database-backed object.
- `save` replaces the complete player record identified by `username` and
  updates `last_saved` in the same write operation. It must not silently change
  the stored password hash unless the API explicitly represents a password
  update.
- `remove` deletes by username. Deleting a missing player is a successful
  no-op unless the public API explicitly chooses a not-found result.

All public operations must report SQLite failures distinctly from validation,
not-found, and authentication failures. Use prepared statements and bound
parameters for every user-provided value.

## Validation and Transactions

- Validate usernames before touching SQLite. Define length, allowed
  characters, whitespace handling, and case sensitivity in the public API and
  apply the same rules to every operation.
- Reject empty serialized data only if the public contract requires it; the
  store should otherwise treat player text as opaque.
- Wrap each mutating operation in a transaction where it contains more than
  one SQL statement. Roll back on every failure.
- Use a single connection per store handle unless the implementation documents
  a different ownership model. If handles can be shared across threads,
  synchronize access or require callers to provide external synchronization.
- Use parameter binding and a busy timeout rather than retry loops scattered
  through individual operations.

## Testing Requirements

Add focused unit tests under `/tests` for project-owned behavior. Tests should
use a temporary SQLite database and exercise real SQL behavior, including:

- creating a new store and reopening an existing one;
- username validation and duplicate usernames;
- successful login updating `last_login`;
- failed login leaving `last_login` unchanged;
- save/restore round trips, including the player-data version;
- replacing an existing record and removing it;
- malformed or unavailable database errors;
- concurrent or locked-writer behavior if the chosen API exposes it.

Avoid tests that only verify mock calls or private implementation details. Keep
SQLite cleanup deterministic and do not use the repository's persistent player
store as test state.

## Build Integration

Provide a dedicated CMake target for the library. Expose only public headers
through the target's public include directories, link SQLite through its
existing project target, and keep SQLite-specific implementation details out
of consumers. Do not make the driver executable a prerequisite for player-store
tests.
