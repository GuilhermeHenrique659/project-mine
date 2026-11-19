# Architecture

## Module map (`src/`)

| Module | Namespace | Role | Details |
|---|---|---|---|
| `main.cpp` | — | Only creates `Game`, calls `start_up()` and `run()`. Tests exclude it. | |
| `game/` | `game` | Window, main loop, active scene, `EventBus`, save rules | below |
| `event/` | `event` | Synchronous type-safe publish/subscribe | [engine-modules.md](engine-modules.md#event) |
| `scene/` | `scene` | MVC scenes: menu, settings, new game, load game, world | below, [world-scene.md](world-scene.md) |
| `scene/world/core/` | `world` | Game logic of the world (ECS content) | [world-logic.md](world-logic.md) |
| `ui/` | `ui` | Raylib-free widget toolkit | [engine-modules.md](engine-modules.md#ui) |
| `input/` | `input` | Keyboard/mouse abstraction, actions | [engine-modules.md](engine-modules.md#input) |
| `persistence/` | `persistence` | Settings and saves on disk | [persistence.md](persistence.md) |
| `core/` | `core` | Generic header-only ECS | [engine-modules.md](engine-modules.md#core) |
| `graphics/` | `graphics` | Textures, sprites, animations | [engine-modules.md](engine-modules.md#graphics) |
| `logging/` | `logging` | `info()` / `warning()` to stderr | [engine-modules.md](engine-modules.md#logging) |
| `text/` | `text` | Number parsing/formatting | [engine-modules.md](engine-modules.md#text) |

### `game/`

`Game` owns the window, the main loop, the active scene and the `EventBus`. It applies display settings to the window, switches scenes, and owns the save rules: it creates, loads and writes saves through `persistence::SaveStore`. `Game(data_dir, clock, seed_source)` takes the clock and seed source so tests get a fixed creation time and seed; a new game draws its world seed from the source.

- `game_events.hpp`: business events `NewGameRequested`, `LoadGameRequested`, `CreateGameRequested`, `LoadSaveRequested`, `SaveGameRequested`, `SettingsRequested`, `BackToMenuRequested`, `DisplaySettingsChanged`, `QuitRequested`.
- `display_settings.hpp`: `DisplaySettings` and the resolution presets.
- `save_game.hpp`: `SaveGame` (name + creation time + world seed + `entities`, a list of `EntityRecord { archetype, label, fields }`: plain strings, so `game/` and `persistence/` know no ECS types), `random_seed()`, `is_valid_save_name()`, `normalize_save_name()`.

### `scene/`

`Scene` composes a `Model`, a `View` and a `Controller` (MVC); model and view may be null. `scene::View` has an optional `update()`, called by `Scene::update()` after the model.

- `panel_view.hpp`: `PanelView` (a menu screen: one widget tree, centred `CardTop` px below the top; `layout(canvas, width)`, `draw(canvas, screen)`, `handle_pointer()`, `handle_scroll()`, `focus_group()`; subclasses build the tree and call `set_root()`) and `PanelController` (injected keyboard, `InputMap` and `input::Pointer`; forwards the click and wheel to the view each frame, binds Up/Down/Confirm to the focus group, `on_cancel()`, `input().on()` for more). Every menu screen uses both.
- `scene/menu/`: title/main menu (`MenuView`, `make_title_scene()`).
- `scene/settings/`: settings screen (`SettingsView`, `make_settings_scene()`).
- `scene/new_game/`: save-name prompt (`NewGameView`, `NewGameController`, `make_new_game_scene()`).
- `scene/load_game/`: save list (`LoadGameView`, `make_load_game_scene()`).
- `scene/world/`: the in-game scene, see [world-scene.md](world-scene.md).

Inside `scene::`, write the top-level namespaces as `::world` and `::core` when a name could be ambiguous (`world` clashes, `core` does not today).

## How the components communicate

```
raylib keyboard ──► input::Keyboard ──► InputHandler ──(Action handlers)──► Controller
raylib mouse ──► input::Pointer ──────────────────────────────────────────► Controller
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
- **Input is mapped in two steps.** Raw keys → `Action` through `InputMap`/`InputHandler`; the controller registers a handler per `Action` and never names a physical key. On the menu screens `scene::PanelController` reads an injected `input::Pointer` and forwards it with `view.handle_pointer(position, released)` / `view.handle_scroll()`; on the world screen the interactive renderers read it themselves through an injected `input::PointerGate` in `View::update()`; the topmost renderer that captures the pointer claims it, so a click never falls through a panel to the world or the build bar.
- **UI events live on the widget.** Every widget accepts `on_click`, `on_hover` and `on_leave` (builder methods or `set_on_*`). `Box` (and so `Card`) forwards pointer events to its visible children; `handle_scroll()` passes the wheel down until a `ScrollBox` takes it. A disabled `Button` ignores clicks.
- **Popups.** A widget with a popup (`Dropdown`) draws it in `draw_overlay()` (the parent draws overlays after all siblings) and returns `blocks_pointer()` while open, so its `Box` sends it all pointer events and clicks never reach widgets underneath.
- **Save flow.** New Game → `NewGameRequested` → name prompt → `CreateGameRequested{name}` → `Game` writes the save and enters the world. Load Game → `LoadGameRequested` → save list → `LoadSaveRequested{name}` → `Game` reads the save and enters the world (stays on the list if it cannot be read). In the world the Save button publishes `SaveGameRequested{save}` with the model's state and `Game` writes it.
- **Text entry.** A screen with a `ui::Input` uses `InputMap::text_entry_defaults()` and its controller forwards `Keyboard::next_char()` to the view each frame; `Action::Erase` (Backspace) erases and confirming the focused field calls its `on_submit`.
- **Keyboard focus** is a `ui::FocusGroup`: add `Focusable` widgets (`Button`, `Input`, `Checkbox`, `Dropdown`), call `focus_next()/focus_previous()/activate_focused()`, and call `focus(widget)` on hover to sync mouse and keyboard (`ui::join_focus_group()` wires both for any control; `ButtonFactory` does it for buttons).
- **Layout and drawing are separate from raylib.** Widgets measure and draw through `ui::Canvas`; `layout()` runs before `draw()` and hit-testing uses the bounds of the last layout.
- **Fixed simulation tick.** `Game::run()` clamps each frame to `game::MaxFrameTime` (0.25 s, `clamp_frame_time()`). `WorldModel::update(dt)` feeds a `game::FixedStep` and runs whole ticks of `scene::world::SimulationTick` (1/60 s), carrying the remainder; systems always get exactly one tick, so results do not depend on the frame rate. Paused time is not banked.
- **Player commands are queued.** `WorldModel::request_build/request_transfer/request_workers` queue their event; it is published at the start of the next tick, before the systems run. The registry only changes inside a tick; a frame with no complete tick applies nothing yet.
- **Scene lifecycle.** `Game::change_scene()` applies at the start of the next `update()`. Per frame: `Scene::update()` runs `Controller::handle_input()`, `Model::update()` then `View::update()`; `Scene::draw()` runs `View::draw()`.

## Adding a screen

1. Build the widget tree in a `View` (a `scene::PanelView` for a menu screen) with the `ui::` builders (`ui::card().child(...)`, or `ui::column()` / `ui::row()` boxes for panels instead of hand-computed positions, `ui::button("...").on_click(...)`); publish business events from the click handlers.
2. Add a `Controller` that turns input into calls on the view through an `input::InputHandler` (one handler per `input::Action`); for a menu screen use `scene::PanelController` (pass it a `RaylibKeyboard` and `RaylibPointer` in the factory, a fake of each in tests). A widget that shows changing game data is a `ui::Composite`: it keeps its tree and rebuilds it only when its shape changes. Never position or draw UI by hand.
3. Put the rules in a `Model` or in an event subscriber, and test them without a window (fake `Keyboard`, `FakeCanvas`, a real `EventBus`).
4. Compose them in a `make_<name>_scene(game::Game&)` factory returning a `scene::Scene`.
