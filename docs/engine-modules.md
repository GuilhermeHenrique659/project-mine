# Engine modules

## core

Generic ECS, header only, raylib-free. `entity.hpp`: `EntityId` and `Entity { id, label }`. `registry.hpp`: `Registry` creates/destroys entities (ids sequential, never reused), attaches components (`add<C>`, `get<C>`, `has<C>`, `remove<C>`) and runs systems over them with `each<Cs...>(fn)`.

## event

Header only. `EventBus` (synchronous, type-safe publish/subscribe), `ScopedSubscription` (RAII unsubscribe) and `subscribe_scoped<E>()`.

## ui

Reusable, raylib-free widget toolkit: `Style` (CSS-like), builders (HTML-like), `Widget` base, `Label`, `Button`, `Input`, `Checkbox`, `Dropdown`, `Card`, `TabBar` (a row of button-like tabs, one active; `theme::tab_style()` / `active_tab_style()` (green) / `disabled_tab_style()`; disabled tabs ignore clicks; `on_change(index)`), `draw_raised()` (paints any area like a button face: fill, outline, bevel), `FocusGroup`/`Focusable` (keyboard navigation), `Canvas` (drawing surface). Only `raylib_canvas.cpp` touches raylib.

## input

- `Key`, `Action`, `Keyboard` (interface), `InputMap` (key → action bindings, rebindable) and `InputHandler` (polls the keyboard, runs the handler of each triggered action; `is_held(Action)` for continuous input such as walking).
- `Keyboard::next_char()` returns the characters typed this frame; `Keyboard::is_down()` is true while a key is held.
- Bindings: `menu_defaults()`; `text_entry_defaults()` (leaves letters and Space unbound); `world_defaults()`: WASD → `MoveUp/MoveDown/MoveLeft/MoveRight`, arrows/Enter/Space navigate/confirm, Esc → `Pause`, Tab → `ToggleInventory`, B → `ToggleBuildMenu`.
- `Pointer` (`pointer.hpp`), the mouse counterpart: `position()`, `released()` (left click), `down()` (left button held), `scroll()` (wheel). The world screen injects it into its renderers; the other screens still read the mouse in their controllers.
- Only `raylib_keyboard.cpp` and `raylib_pointer.cpp` touch raylib.

## graphics

- `TextureManager`, a singleton (`TextureManager::instance()`): `Game::start_up()` registers each texture as name → file (`register_texture()`) and installs the loader (`set_loader()`). `get(name)` loads on first use and, when the name is not registered or the file cannot be read, returns the **fallback texture** (a generated magenta/black checkerboard, created once through `TextureLoader::create_placeholder()`, null only when there is no loader). `find(name)` is the same lookup without the fallback; `fallback()` gives the fallback itself.
- Drawing: `ui::Canvas::draw_texture(texture, dest, TextureDraw{source, tint, flip_x, flip_y, rotation})` (the 2-argument overload draws the whole texture).
- `Sprite` (`sprite.hpp`: texture name + optional source region) with `draw_sprite(canvas, sprite, dest, options)`, which draws the whole fallback instead of a missing texture's region and returns false only when there is no fallback either.
- `SpriteSheet` (`frame(i)` / `frames(first, count)`, numbered left to right, top to bottom) and `Animation` (`sprite_at(elapsed_seconds)`, fixed fps, loop or hold the last frame; keeps no clock).
- `TextureLoader` is the interface; only `raylib_texture_loader.cpp` touches raylib. Tests install a fake loader and call `reset()` in set-up/tear-down.

## logging

`logging::info()` / `logging::warning()` write to stderr (`[INFO]` / `[WARN]`); `set_sink()` redirects messages (tests capture them). Raylib-free.

## text

Header only, raylib-free, usable from any module (including `world::`). `parse.hpp`: `text::parse<T>(text)` reads the whole text as an integer or floating-point `T` (empty on anything else, a '+', spaces, or overflow; unsigned rejects '-'; locale-independent), and `text::format(value)` writes the shortest text that `parse<T>()` reads back exactly. Use them for every number read from or written to a file instead of hand-written loops or `std::sto*`.
