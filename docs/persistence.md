# Persistence (`persistence/`)

Saves user data to disk. Raylib-free.

## Data directory

`resolve_data_dir()`: `--data-dir <path>` > env `PROJECT_MINE_DATA_DIR` > build default (Debug: `resource/` relative to the working dir, Release: `%APPDATA%/ProjectMine`). For dev runs, `.env` in the project root (`PROJECT_MINE_DATA_DIR=resource`) is read at startup.

## Settings

- `SettingsStore` reads/writes `settings.cfg` (`key=value`, written via a temp file + rename).
- `SettingsPersistence` subscribes to `DisplaySettingsChanged` on the `EventBus` and saves.
- `Game(data_dir)` loads the saved settings at startup and opens the window with them.

## Saves

`SaveStore` reads/writes saved games as `saves/<name>.sav` (`key=value`): `name`, `created_at`, `seed` and the entities.

- Format version 2 writes each record as an `[entity]` section of `archetype=`, `label=` and `key=value` fields after the header. A record without archetype is dropped; field keys `archetype`/`label` and keys/values with line breaks are not written.
- A file with a newer `version` is not loaded (so saving cannot drop its unknown data). A v1 file or a save without a seed loads with no entities / seed 0.
- Every save first moves the existing file to `<name>.sav_old` (replacing the previous backup); `load()` falls back to that backup when the current file is missing or corrupt.
- `list()` returns the loadable saves newest first.

`write_file_atomically()` (`atomic_file.hpp`) is the shared temp file + rename (+ optional backup) write.

How ECS entities become `EntityRecord`s is in [world-logic.md](world-logic.md#saving-entities).
