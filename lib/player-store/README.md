# Player Store

A library of MUD system player store using `sqlite3`.

Basic operations are:
- `open`: opens an existing player store, or create one if not existing.
- `login`: atomic operation to query for (username, password_hash) and update last_login on success.
- `restore`: restores a copy of player's saved data from the player store.
- `save`: save a player's data. Overwrites existing.
- `remove`: remove a player's data.

Each player's data contains below members:
- `username`: a unique string to identify the player. The player store API does check of required length or allowed characters in the username.
- `password_hash`: hashed password in PHC string format `$id$params$salt$hash`
- `last_login`: timestamp when last successful login
- `last_saved`: timestamp when last time saved
- `player_text`: player data as serialized in printable text (usually JSON) 
