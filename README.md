# project-mine

A Factorio-style game with RTS elements, written in C++20 with [raylib](https://www.raylib.com/).

## Current features

- Main menu with New Game, Load Game and Settings (resolution and fullscreen, saved to `settings.cfg`).
- Named saves with a backup (`<name>.sav_old`) kept on every save.
- A seed-generated isometric world: grass, water, and iron, copper and coal ore nodes.
- A player you walk with WASD, a camera that follows them, and mouse-wheel zoom.
- A 40-slot inventory and a player menu (Tab) with Inventory and Energy tabs.
- A build bar (B) with building previews, construction sites you build by hand, a Headquarters and Mines.
- Mines that produce the ore under them, and a power grid: the Headquarters supplies solar power and mines consume it.

The full description is in [docs/gameplay.md](docs/gameplay.md).

## Controls

| Key | Action |
|---|---|
| WASD | Move |
| Mouse wheel | Zoom |
| Tab | Player menu |
| B | Build bar |
| Left click | Place a building / build a site / open a building |
| Esc | Pause menu / close panels |

## Building

You need MSYS2 UCRT64 (g++, ninja), CMake and GoogleTest:

```
pacman -S mingw-w64-ucrt-x86_64-gtest
```

Then, from a UCRT64 shell:

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

This builds `build/project-mine.exe` (the game) and `build/project-mine-tests.exe` (the tests). raylib, enet and tmxlite are bundled in `include/` and `lib/`.

## Running

Start the game from the project root. The `.env` file there (`PROJECT_MINE_DATA_DIR=resource`) sets the data directory, where assets, settings and saves live. A real environment variable or `--data-dir <path>` overrides it.

```
build/project-mine.exe
```

## Project layout

| Path | Contents |
|---|---|
| `src/` | Game code |
| `tests/` | GoogleTest tests, mirroring `src/` |
| `include/`, `lib/` | Bundled libraries (raylib, enet, tmxlite) |
| `resource/` | Assets (not in git) |
| `docs/` | Reference documentation |

## Documentation

- [docs/index.md](docs/index.md): gameplay, architecture and per-module reference.
- [AGENTS.md](AGENTS.md): contribution rules (architecture, tests, coding standards).
