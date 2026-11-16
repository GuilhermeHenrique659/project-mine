# AGENTS.md

## Project

A Factorio-style game with RTS elements, written in C++20 with raylib.

- `src/` — game code
- `tests/` — tests (GoogleTest), mirroring `src/`
- `include/`, `lib/` — libraries already bundled in the project: raylib, enet, tmxlite (MinGW)
- `resource/` — assets (outside git)
- `build/` — CMake output (outside git)
- `docs/` — reference documentation: start at [docs/index.md](docs/index.md) (gameplay, architecture, per-module details). Update the relevant page when you change the behaviour it describes.

## Build

Requires MSYS2 UCRT64 (g++, ninja), CMake and GoogleTest (`pacman -S mingw-w64-ucrt-x86_64-gtest`).

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

- Game: `build/project-mine.exe`; tests: `build/project-mine-tests.exe`.
- Dev run: start the game from the project root; `.env` there (`PROJECT_MINE_DATA_DIR=resource`) is read at startup. A real environment variable or `--data-dir` overrides it.

The project path contains an accent ("Área de Trabalho") and the MinGW linker does not open absolute paths with accents; that is why `CMakeLists.txt` passes `lib/` as a relative path. Keep it this way when adding libraries.

## Architecture rules

Full module map and data flow: [docs/architecture.md](docs/architecture.md). The rules to respect:

- **Layers publish events instead of calling each other for business rules.** Views/widgets only announce what the player asked for on the `EventBus` (owned by `Game`); the owner of the rule subscribes and reacts. No game rules in buttons or views.
- Subscribers that can be destroyed before the bus hold a `ScopedSubscription`, never a bare `subscribe` id.
- Controllers bind `input::Action`s, never physical keys.
- Only the `raylib_*.cpp` files touch raylib. `ui/`, `core/`, `persistence/`, `logging/`, `text/` and `scene/world/core/` are raylib-free; `scene/world/core/` (namespace `world`) also never includes `ui/` or anything else of `scene/`.
- Dependencies (registry, bus, input, pointer) are injected through constructors, so everything is testable without a window.
- Read and write numbers in files with `text::parse<T>()` / `text::format()`.

## Tests

- **Given / When / Then structure**: every test is organized into three blocks, in this order — *Given* (initial state), *When* (tested action), *Then* (checks).

  ```cpp
  TEST(Inventory, AddItemIncreasesCount) {
      // Given
      Inventory inventory;

      // When
      inventory.add(Item::IronPlate, 3);

      // Then
      EXPECT_EQ(inventory.count(Item::IronPlate), 3);
  }
  ```

- **Only meaningful tests**: each test must verify a real game behavior (rule, calculation, state transition, edge case). Do not create trivial tests or tests written just to increase coverage (getters/setters, constructors, library code).
- Shared fakes (canvas, keyboard, pointer, input, temp dir) are in `tests/support/`, see [docs/testing.md](docs/testing.md).

## Coding standards

- **Smart pointers are mandatory**: all dynamic memory ownership uses `std::unique_ptr` (default) or `std::shared_ptr` (only when ownership is truly shared). Do not use `new`/`delete` or raw pointers as owners; create objects with `std::make_unique`/`std::make_shared`.
- **Early return whenever possible**: handle invalid and exit cases at the start of the function and return early, instead of nesting `if`/`else`.
- **Naming conventions** (common modern C++ community style):

  | Element | Style | Example |
  |---|---|---|
  | Types (classes, structs, enums, aliases, concepts) | `PascalCase` | `ConveyorBelt`, `ItemStack` |
  | Functions and methods | `snake_case` | `add_item()`, `update_power()` |
  | Variables and parameters | `snake_case` | `tile_size`, `current_tick` |
  | Private/protected data members | `snake_case` + trailing `_` | `items_`, `health_` |
  | Public struct fields | `snake_case` (no suffix) | `position`, `speed` |
  | Constants (`constexpr`/`const` globals) and enumerators | `PascalCase` | `MaxStackSize`, `Item::IronPlate` |
  | Macros | `UPPER_SNAKE_CASE` (avoid when possible) | `PROJECT_MINE_DEBUG` |
  | Namespaces | `snake_case`, short | `world`, `net` |
  | Template parameters | `PascalCase` | `T`, `Allocator` |
  | Files | `snake_case`, `.hpp` / `.cpp` | `conveyor_belt.hpp` |

  Use `enum class` instead of plain `enum`. Test names (GoogleTest suite and case) use `PascalCase`, as in the example above.
