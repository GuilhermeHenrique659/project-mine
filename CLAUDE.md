# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Project rules (overview, build, test structure, coding and naming standards) live in AGENTS.md and must be followed:

@AGENTS.md

## Commands

Run from an MSYS2 UCRT64 environment:

```
cmake -S . -B build -G Ninja          # configure (defaults to Release)
cmake --build build                   # build game + tests
ctest --test-dir build --output-on-failure
build/project-mine-tests.exe --gtest_filter=Suite.Case   # single test (wildcards allowed: Suite.*)
```

Add `-DCMAKE_BUILD_TYPE=Debug` when configuring for a debug build. No linter is configured; the compiler runs with `-Wall -Wextra`.

## Build layout (CMakeLists.txt)

- Sources are picked up by `GLOB_RECURSE ... CONFIGURE_DEPENDS` over `src/*.cpp` and `tests/*.cpp`: new files need no CMake edits.
- The test executable compiles **all of `src/` except `src/main.cpp`** together with `tests/`. Keep `main()` and nothing else that tests need in `src/main.cpp`; put testable logic in other files.
- Both targets share `setup_target()`: `src/` and `include/` (as SYSTEM) on the include path, raylib + `opengl32 gdi32 winmm`, fully `-static` link.
- GoogleTest is the MSYS2 package, found next to the compiler (`<compiler>/../lib`), not fetched.
- enet and tmxlite are bundled but **not linked yet**. To use them, add `tmxlite` / `enet ws2_32` to `setup_target()` (and copy `lib/libenet-7.dll` next to the exe for enet).
- `lib/` is passed to the linker as a path relative to the build dir because of the accented project path (see AGENTS.md). Don't replace it with an absolute path.

## Current state

The game opens a window with the main menu (New Game, Load Game, Settings, Quit). Quit works and Settings opens a screen with a resolution dropdown and a fullscreen checkbox, applied to the window by `Game` and saved to `settings.cfg` in the data directory (see `persistence/` in AGENTS.md), so they are restored on the next run. New Game asks for a save name, writes the save (`saves/<name>.sav` in the data directory, overwriting a save with the same name) and enters the world scene. Load Game lists the saves, newest first, and enters the world scene with the chosen one. The world scene (`scene/world/`) shows, only while paused (Esc), a pause menu with the save's name and creation date, a Save button that writes the save again (keeping the previous file as `<name>.sav_old`) and a Main Menu button, over an isometric view of the ECS world (`core/`, `scene/world/core/`): a Player character, drawn as a cube labelled with its name and state (`Idle` / `Moving`) walks with WASD (the camera follows it, so it stays at the screen centre) (diagonals are not faster, it stops on release); a `CharacterStateChanged` event is published when it starts or stops moving. The player carries a 40-slot `Inventory` (slots of item stacks, `world::items::` definitions), saved slot by slot; Tab opens the player menu (the world keeps running; Esc, Tab or its X button closes it), a card over most of the screen with tabs: Inventory (active, the slot grid) and Statistics / Logistics (disabled). Nothing puts items in the inventory yet. B shows or hides the build bar of the player HUD at the bottom: icon tabs Special / Mines / Factories / Military, each opening a drop-up list of constructions (icon + name) above it; only the Headquarters (free, built at once, only one) and the Mine (100 iron ore, effort 300, only on an ore tile, only once a Headquarters exists) can be picked; the others have no building type yet and are disabled. With one selected, a translucent cube previews it on the tile under the mouse (green if it can be built there, red if not) and clicking the world places it (a refused click keeps the selection and turns the preview a stronger red), drawn as a tile-wide cube (blue when finished, grey with a % while a construction site). Holding the left mouse button over a site within 2 tiles of the player builds it at 10 effort/s (a mine takes 30 s). A new game's player starts with 200 iron ore. All icons are the placeholder (fallback) texture. There is no other gameplay yet, and a save holds name, creation time, the world seed and the persistent entities (the Player: its position is saved and restored on load, and its state is always saved as `Idle`; and buildings: type, tile and construction progress); the ground is a flat seed-generated world of green grass, blue water and iron (grey), copper (orange) and coal (black) ore nodes (three medium ones near the spawn, bigger ones further out), recreated from the seed on load. The save list does not scroll.

Module layout and how the layers communicate (event bus, input actions, widget events, focus) are documented in AGENTS.md under "Architecture". After adding or removing source files, rerun the configure step.
