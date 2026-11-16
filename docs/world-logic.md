# World logic (`scene/world/core/`, namespace `world`)

The ECS content of the world, built on the generic `core/` ECS. It lives under `scene/world/` but is raylib-free and view-free: it never includes `ui/`, raylib or anything else of `scene/` (not `render/`, `WorldModel`, `WorldView`); it may use `input/` actions, injected as an `input::InputHandler`. Subfolders add no namespace, and the folder name is not the namespace (`world::`; do not confuse it with the generic `core::`).

## Systems

`system.hpp`: the `System` base class (`update(dt)`; a system gets the registry and bus in its constructor). Order in `WorldModel`'s list is order of execution:

1. `PlayerInputSystem(registry, input)`: sets the `Mover` direction of `PlayerControlled` entities from the held `Move*` actions (screen space turned into world space).
2. `CharacterMoveSystem(registry, bus)`: moves entities with `Character` + `Position` + `Mover` (normalized direction, so diagonals are not faster), sets the state and publishes `CharacterStateChanged{entity}` only when a character starts or stops moving.
3. `CameraFollowSystem(registry, camera)`: centres the camera on the `PlayerControlled` entity.
4. `BuildSystem`: builds on `BuildRequested`, paid by the player. `ConstructionWorkSystem`: each `Builder` within `BuildReach` of its site adds `rate * dt`; at the effort the site is removed, its builders stop and `BuildingCompleted` is published.
5. `InventoryTransferSystem`: on `ItemTransferRequested{from, to, item, count}` runs `transfer_items()` when both have an `Inventory` and are within `TransferReach` (2 tiles; `within_transfer_reach()`).
6. `ProductionSystem(registry, terrain)`: finished buildings with `Producer` + `Inventory`: progress += rate * dt, each whole unit makes one output into the building's inventory; waits with at most one unit banked while the input is missing or the output does not fit. `item_from_tile()`. Production progress is not saved.

## Components

- `Character` (marks a character, holds its `CharacterState`: `Idle` or `Moving`), `Position` (a `WorldPoint`), `Mover` (speed in tiles/s + direction intent), `PlayerControlled`.
- `Inventory` (`inventory.hpp` + `.cpp`): a fixed number of `ItemStack` slots; `add()` returns what did not fit and tops up same-type stacks before using empty slots; `remove()` returns what it took, from the last slots first; `count()`, `free_slots()`. `transfer_items(from, to, item, count)` moves as much as the source holds and the target fits and returns how many moved.
- `building.hpp`: `Building { type, tile }`, `ConstructionSite { progress, effort }` (present while unfinished), `Producer { recipe, progress }` (its own copy of the recipe so the rate can change per building).
- `builder.hpp`: `Builder { rate, site }` (`CharacterBuildRate` 10/s, `BuildReach` 2 tiles; every character has one).
- `persistent.hpp`: `Persistent{archetype}`; only entities with it are saved.

## Items

`items/item.hpp`: `ItemType` (`IronOre`, `CopperOre`, `Coal`), `Item { type, max_stack }` (definitions in `items::`, 100 each), `ItemStack { item, count }`, `to_string(ItemType)`.

## Entities

- `entities/character.hpp`: `spawn_character()` (`"character"` archetype): `Character` + `Position` + `Mover` at `CharacterSpeed` + an `Inventory` of `CharacterInventorySlots` (40) + a `Builder`. A new game's player gets `StartingIronOre` (200).
- `entities/building.hpp`: `spawn_building()` (`"building"` archetype) and `make_building()`: `Building` + `Position` at the tile centre + a site when effort > 0 + an `Inventory` of `inventory_slots` when > 0 + a `Producer` when it has a recipe.
- `entities/entity_factory.hpp`: `EntityFactory` (archetype name → spawn function, injected; `make_entity_factory()` registers them; `spawn()` adds `Persistent`). `WorldModel` spawns the new-game player through it too.

## Events

`events/character_state_changed.hpp` (`CharacterStateChanged`), `events/building_events.hpp` (`BuildRequested{type, tile}`, `BuildingCompleted{entity}`), `events/item_transfer_requested.hpp` (`ItemTransferRequested`).

## Construction

- `construction/construction.hpp`: `ConstructionCategory` (`Special`, `Mine`, `Factory`, `Military`); `BuildingType` (`Headquarters`, `Mine`; one value per implemented building; `building_type_key()` / `parse_building_type()` for saves); `Construction { name, icon, type, cost, effort, unique, needs_headquarters, needs_ore, inventory_slots, recipe }` where `type` empty = listed but not buildable yet, `cost` = item stacks paid from the player's inventory, `effort` = work units, 0 = finished at once; `Recipe { input, output, rate }` (every 1/`rate` s consumes the optional `input` stack and adds one `output`; empty `output` = the ore under the building); `ConstructionGroup`; `construction_catalog()` (by category, in build bar order) and `find_construction(type)`.
  - Headquarters: free, effort 0, unique, the only entry that does not need a headquarters.
  - Mine: 100 iron ore, effort 300, needs an ore tile, 10-slot inventory, recipe: no input, the ore below, 0.5/s.
- `construction/build_rules.hpp`: `BuildResult`; `has_headquarters()`, `check_unlocked()` (NotBuildable / NeedsHeadquarters / AlreadyExists), `can_build()` (+ TileBlocked: water or another building, NeedsOre, NotEnoughResources), `build()` (applies them, pays the cost, spawns through the factory).

## Terrain

`terrain/terrain.hpp`: `Terrain(seed)`, a flat unbounded world of `TileType::Grass` / `Water` / `IronOre` / `CopperOre` / `Coal` (`is_ore()`). `tile_at(tile)` is a pure function of the seed (value noise), so a loaded save recreates the same world:

- the `SpawnClearRadius` around (0,0) is always grass;
- three medium starting ore nodes, one per type, 24–36 tiles out, each on the first dry spot around its seed-random angle (when none is fully dry, the driest one is used and its footprint turned into land), so ore never touches water;
- scattered nodes, one per 64-tile cell, only on land, none inside `StartingAreaRadius` (64), radius growing with the distance from (0,0) up to a cap; irregular edges via the noise.

## Saving entities

- `save/component_codec.hpp`: `ComponentCodec` (`write` / `read` of one component's `Fields`).
- `save/codecs.hpp`: `PositionCodec`, `CharacterCodec` (writes and restores `Moving`/`Running` as `Idle` through `saved_state()`), `PlayerControlledCodec`, `InventoryCodec` (`inventory.<slot>=<ItemType>:<count>` per filled slot, invalid entries skipped), `BuildingCodec` (`building.type`, `building.x`, `building.y`; re-places the building), `ConstructionSiteCodec` (`construction.progress`; missing = finished).
- `save/world_serializer.hpp`: `WorldSerializer` (`snapshot()` / `restore()`); `make_world_serializer(factory)` lists the codecs: a new saved component = one codec there. `BuildingCodec` comes first because re-placing a building recreates its components. Entity ids are not saved. This is the one place where `world::` depends on `game/save_game.hpp` (plain data).

## Camera and isometric math

- `camera.hpp`: `Camera` (`center`, `projection(w, h)` puts the centre at the middle of the screen, `MinZoom`/`MaxZoom`). `TerrainRenderer` and `EntityRenderer` draw through its projection.
- `isometric.hpp` (2:1 diamond projection): `TileCoord`, `WorldPoint` (continuous, in tiles), `ScreenPoint`, `IsoProjection` (tile size + `origin`, the screen position of world (0,0): move it to scroll, scale the tile size to zoom) with `world_to_screen()`, `screen_to_world()`, `tile_at()` (mouse picking), `tile_center()`, `tile_corners()`; `tile_containing()` floors a world position to its tile; `depth_of()` / `draws_before()` give the back-to-front order; `screen_direction_to_world()` (screen up = world (-1,-1)).
