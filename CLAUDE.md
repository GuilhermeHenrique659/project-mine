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

The game opens a window with the main menu (New Game, Load Game, Quit). Quit works; New Game and Load Game only publish `NewGameRequested` / `LoadGameRequested`, nothing consumes them yet because there is no gameplay scene or save system.

Module layout and how the layers communicate (event bus, input actions, widget events, focus) are documented in AGENTS.md under "Architecture". After adding or removing source files, rerun the configure step.
