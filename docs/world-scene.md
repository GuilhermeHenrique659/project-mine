# World scene (`scene/world/`)

## `WorldModel`

- `WorldModel(bus, input, save)` owns the `core::Registry`, the `Camera` (`camera()`), the `Terrain` built from `save.seed` (`terrain()`), and its `world::System`s in a `std::vector<std::unique_ptr<::world::System>>`, run in order in `update()`.
- It passes the `input::InputHandler` to `world::PlayerInputSystem`, so movement never goes through the controller. `make_world_scene(game, save)` creates the handler and shares it with the controller, which only handles the pause/menu actions and zooms the camera with the mouse wheel (`Camera::MinZoom`/`MaxZoom`).
- Restores the entities in its constructor (spawning the default player through the `EntityFactory` when the save has none or no player) and centres the camera on the player.
- `save_game()` returns the header plus a fresh snapshot; `save_header()` is the header alone. Not copyable or movable (the serializer and systems refer to its members).
- **Pause**: `toggle_pause()`; no simulation while paused. `cancel()` (Esc) closes the player menu first, otherwise toggles the pause.
- **Player menu**: `toggle_player_menu()` (ignored while paused), `close_player_menu()`, `is_player_menu_open()`; the simulation keeps running while it is open.
- **Build bar**: `toggle_build_bar()` (ignored while paused), `build_status()`, `placement_status()` (every rule incl. tile and cost), `request_build(type, tile)` (publishes `BuildRequested`).
- **Work and transfer**: `work_on(tile | nullopt)`; `open_transfer(tile)` (a finished building with an inventory within `TransferReach`), `transfer_target()`, `close_transfer()` (also closed by `update()` when out of reach and by `cancel()` first), `request_transfer()`.

## `WorldView` and renderers

`WorldView` hosts a list of `scene::world::Renderer`s (`renderer.hpp`: `visible()`, `update()` polls input, `draw(canvas, screen)`), built by `make_world_scene` and injected. Drawn in list order (first = bottom), updated topmost first, skipped while not `visible()`. A renderer gets its dependencies (registry, bus, input) in its constructor, like a model system. Renderers live in `render/`; in draw order:

- `TerrainRenderer(terrain, camera)`: the visible tiles as isometric diamonds; `tile_color(TileType)`.
- `EntityRenderer(registry, camera)`: each entity with a `Position` as a `Cube`, with its label and, for characters, the state. Buildings are tile-wide (`BuildingCubeSize`) blue cubes, grey with a progress % while a construction site. No textures yet.
- `SiteWork(model, pointer)`: draws nothing; calls `work_on(tile)` each frame while the left button is held over the world, `work_on(nullopt)` otherwise (pausing and the player menu also stop work); a click calls `open_transfer(tile)`. Hidden while a transfer is open.
- `PlayerHud(model, pointer)`: the build bar: icon tabs at the bottom centre, one per `construction_catalog()` category; a tab toggles a drop-up list (icon + name); clicking a construction selects it (`selected()`) and closes the list; entries refused by `build_status()` are disabled. With a selection, a world click requests the build and clears it; `preview()` gives the tile under the pointer and whether it can be built (translucent green/red cube); a refused click keeps the selection and marks the tile `rejected` until the pointer leaves it. Icons are unregistered texture names (fallback texture).
- `BuildingPanel(model, pointer)`: the player's `InventoryGrid` above the building section: its name, its `building_details()` lines (`render/building_details.hpp`: "State: Working" / "State: Idle (no power | no ore | missing input | output full)", "Power: N kW", "Generates: N kW", one per component it has) and its `InventoryGrid`; a click on a filled slot moves its stack across; X closes.
- `PlayerMenu(model, pointer)`: panel over 90% of the screen with a `ui::TabBar` (Inventory active, Energy, Statistics/Logistics disabled), a red X in the top-right corner, and the Inventory page: `InventoryGrid(registry, entity)` (rows of 10 button-like slots, `theme::slot_style()`, hovered slot highlighted, `item_color()` + count). Visible while open and not paused.
- `EnergyPage(registry)` (Energy tab): text read from `::world::power_report()` on every draw: "Produced: N kW", "Consumed: N kW" (+ a red overload warning), then "Generators (n)" and "Consumers (n)", one indented "<name> - N kW" line each (red with " (stopped)" / " (no power)" when inactive, "None" when empty).
- `WorldMenu(bus, model, pointer)`: pause menu (save name/date, Save, Main Menu), visible only while paused; Save calls `save_game()`, so the saved position is the one at click time.

`render/cube.hpp`, `Cube(size)`: a reusable isometric box in the tiles' 2:1 projection, `size` pixels wide, standing on its ground point (the centre of its base diamond); top / left / darker right faces drawn as triangles; `faces(ground)`, `height()`, `bounds(ground)`.
