# AGENTS.md

## Project

A Factorio-style game with RTS elements, written in C++20 with raylib.

- `src/` — game code
- `tests/` — tests (GoogleTest)
- `include/`, `lib/` — libraries already bundled in the project: raylib, enet, tmxlite (MinGW)
- `resource/` — assets (outside git)
- `build/` — CMake output (outside git)

## Build

Requires MSYS2 UCRT64 (g++, ninja), CMake and GoogleTest (`pacman -S mingw-w64-ucrt-x86_64-gtest`).

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

- Game: `build/project-mine.exe`
- Tests: `build/project-mine-tests.exe`
- Dev run: start the game from the project root; `.env` there (`PROJECT_MINE_DATA_DIR=resource`) is read at startup, so settings are saved in `resource/settings.cfg`. A real environment variable or `--data-dir` overrides it.

The project path contains an accent ("Área de Trabalho") and the MinGW linker does not open absolute paths with accents; that is why `CMakeLists.txt` passes `lib/` as a relative path. Keep it this way when adding libraries.

## Architecture

### Code structure (`src/`)

| Module | Namespace | Role |
|---|---|---|
| `main.cpp` | — | Only creates `Game`, calls `start_up()` and `run()`. Tests exclude it. |
| `game/` | `game` | `Game` owns the window, the main loop, the active scene and the `EventBus`. `game_events.hpp` holds the business events (`NewGameRequested`, `LoadGameRequested`, `CreateGameRequested`, `LoadSaveRequested`, `SaveGameRequested`, `SettingsRequested`, `BackToMenuRequested`, `DisplaySettingsChanged`, `QuitRequested`); `display_settings.hpp` has `DisplaySettings` and the resolution presets; `save_game.hpp` has `SaveGame` (name + creation time) and the save-name rules (`is_valid_save_name()`, `normalize_save_name()`). `Game` applies display settings to the window, switches scenes, and owns the save rules: it creates, loads and writes saves through `persistence::SaveStore` (`Game(data_dir, clock)` takes the clock so tests get a fixed creation time). |
| `event/` | `event` | `EventBus` (synchronous, type-safe publish/subscribe), `ScopedSubscription` (RAII unsubscribe) and `subscribe_scoped<E>()`. Header only. |
| `scene/` | `scene` | `Scene` composes a `Model`, a `View` and a `Controller` (MVC). Model and view may be null. `scene/menu/` is the title/main menu scene (`MenuView`, `make_title_scene()`); `scene/settings/` is the settings screen (`SettingsView`, `make_settings_scene()`); `scene/new_game/` asks for the save name (`NewGameView`, `NewGameController`, `make_new_game_scene()`); `scene/load_game/` lists the saves (`LoadGameView`, `make_load_game_scene()`); `scene/world/` is the main in-game scene (`WorldModel`, `WorldView`, `make_world_scene(game, save)`). Inside `scene::`, a top-level `world` namespace must be written `::world`. |
| `ui/` | `ui` | Reusable, raylib-free widget toolkit: `Style` (CSS-like), builders (HTML-like), `Widget` base, `Label`, `Button`, `Input`, `Checkbox`, `Dropdown`, `Card`, `FocusGroup`/`Focusable` (keyboard navigation), `Canvas` (drawing surface). Only `raylib_canvas.cpp` touches raylib. |
| `input/` | `input` | Keyboard abstraction: `Key`, `Action`, `Keyboard` (interface), `InputMap` (key → action bindings, rebindable; `menu_defaults()` for menus, `text_entry_defaults()` for screens with a text field, which leaves letters and Space unbound) and `InputHandler` (polls the keyboard, runs the handler of each triggered action). `Keyboard::next_char()` returns the characters typed this frame for text fields. Only `raylib_keyboard.cpp` touches raylib. |
| `persistence/` | `persistence` | Saves user data to disk. `SettingsStore` reads/writes `settings.cfg` (`key=value`, written via a temp file + rename); `SettingsPersistence` subscribes to `DisplaySettingsChanged` on the `EventBus` and saves; `resolve_data_dir()` picks the data directory: `--data-dir <path>` > env `PROJECT_MINE_DATA_DIR` > build default (Debug: `resource/` relative to the working dir, Release: `%APPDATA%/ProjectMine`). `Game(data_dir)` loads the saved settings at startup and opens the window with them. `SaveStore` reads/writes saved games as `saves/<name>.sav` (`key=value`): every save first moves the existing file to `<name>.sav_old` (replacing the previous backup), `load()` falls back to that backup when the current file is missing or corrupt, and `list()` returns the loadable saves newest first. `write_file_atomically()` (`atomic_file.hpp`) is the shared temp file + rename (+ optional backup) write. Raylib-free. |
| `world/` | `world` | Isometric world math (2:1 diamond projection), raylib-free. `isometric.hpp` has `TileCoord`, `WorldPoint` (continuous, in tiles), `ScreenPoint` and `IsoProjection` (tile size + `origin`, the screen position of world (0,0): move it to scroll, scale the tile size to zoom) with `world_to_screen()`, `screen_to_world()`, `tile_at()` (mouse picking), `tile_center()` and `tile_corners()`; `tile_containing()` floors a world position to its tile; `depth_of()` / `draws_before()` give the back-to-front draw order. |
| `logging/` | `logging` | `logging::info()` / `logging::warning()` write to stderr (`[INFO]` / `[WARN]`); `set_sink()` redirects messages (tests capture them). Used by `persistence/` for settings and save-game load/save. Raylib-free. |

`tests/` mirrors the `src/` modules (`tests/ui/`, `tests/persistence/`, `tests/scene/menu/`, ...); shared helpers live in `tests/support/` and are included as `"support/..."`: `fake_canvas.hpp` is a recording `Canvas` for widget tests, `fake_keyboard.hpp` a `Keyboard` whose pressed keys and typed characters the test sets, `temp_dir.hpp` a temporary directory for persistence tests.

### How the components communicate

```
raylib keyboard ──► input::Keyboard ──► InputHandler ──(Action handlers)──► Controller
raylib mouse ────────────────────────────────────────────────────────────► Controller
                                                                              │
                     Controller: view.handle_pointer(), focus_group().focus_next()/activate_focused()
                                                                              ▼
                      ui widgets (Button, ...)  ──on_click/on_hover/on_leave──► MenuView
                                                                              │ publishes business event
                                                                              ▼
                                       game::Game::event_bus()  (EventBus, owned by Game)
                                                                              │
                                  subscribers react (Game quits on QuitRequested, opens the
                                  New Game / Load Game screens, creates, loads and saves games)
```

- **Layers do not call each other for business rules; they publish events.** A view/widget only announces what the player asked for (`bus.publish(game::QuitRequested{})`). The owner of the rule subscribes and reacts. Do not put game rules in buttons or views.
- **The `EventBus` lives in `Game`** (`game.event_bus()`), is shared by every scene, and is declared first in `Game` so it outlives scenes. Subscribers that can be destroyed before the bus must hold a `ScopedSubscription` (never a bare `subscribe` id).
- **Input is mapped in two steps.** Raw keys → `Action` through `InputMap`/`InputHandler`; the controller registers a handler per `Action` and never names a physical key. The mouse is read directly by the controller and forwarded with `view.handle_pointer(position, released)`.
- **UI events live on the widget.** Every widget accepts `on_click`, `on_hover` and `on_leave` (builder methods or `set_on_*`). `Card` forwards pointer events to its children. A disabled `Button` ignores clicks.
- **Popups.** A widget with a popup (`Dropdown`) draws it in `draw_overlay()` (the parent draws overlays after all siblings) and returns `blocks_pointer()` while open, so `Card` sends it all pointer events and clicks never reach widgets underneath.
- **Save flow.** New Game → `NewGameRequested` → name prompt → `CreateGameRequested{name}` → `Game` writes the save and enters the world scene. Load Game → `LoadGameRequested` → save list → `LoadSaveRequested{name}` → `Game` reads the save and enters the world scene (stays on the list if it cannot be read). In the world scene the Save button publishes `SaveGameRequested{save}` with the model's state and `Game` writes it.
- **Text entry.** A screen with a `ui::Input` uses `InputMap::text_entry_defaults()` and its controller forwards `Keyboard::next_char()` to the view each frame; `Action::Erase` (Backspace) erases and confirming the focused field calls its `on_submit`.
- **Keyboard focus** is a `ui::FocusGroup`: add `Focusable` widgets (`Button`, `Input`, `Checkbox`, `Dropdown`), call `focus_next()/focus_previous()/activate_focused()`, and call `focus(widget)` on hover to sync mouse and keyboard (`ui::join_focus_group()` wires both for any control; `ButtonFactory` does it for buttons).
- **Layout and drawing are separate from raylib.** Widgets measure and draw through `ui::Canvas`; `layout()` runs before `draw()` and hit-testing uses the bounds of the last layout.
- **Scene lifecycle.** `Game::change_scene()` applies at the start of the next `update()`. Per frame: `Scene::update()` runs `Controller::handle_input()` then `Model::update()`; `Scene::draw()` runs `View::draw()`.

### Adding a screen

1. Build the widget tree in a `View` with the `ui::` builders (`ui::card().child(...)`, `ui::button("...").on_click(...)`); publish business events from the click handlers.
2. Add a `Controller` that turns input into calls on the view through an `input::InputHandler` (register one handler per `input::Action`).
3. Put the rules in a `Model` or in an event subscriber, and test them without a window (fake `Keyboard`, `FakeCanvas`, a real `EventBus`).
4. Compose them in a `make_<name>_scene(game::Game&)` factory returning a `scene::Scene`.

## Tests

- **Given / When / Then structure**: every test is organized into three blocks, in this order — *Given* (initial state), *When* (tested action), *Then* (checks).

  ```cpp
  TEST(Inventario, AdicionarItemAumentaQuantidade) {
      // Given
      Inventario inventario;

      // When
      inventario.adicionar(Item::PlacaDeFerro, 3);

      // Then
      EXPECT_EQ(inventario.quantidade(Item::PlacaDeFerro), 3);
  }
  ```

- **Only meaningful tests**: each test must verify a real game behavior (rule, calculation, state transition, edge case). Do not create trivial tests or tests written just to increase coverage (getters/setters, constructors, library code).

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
