# World logic (`scene/world/core/`, namespace `world`)

The ECS content of the world, built on the generic `core/` ECS. It lives under `scene/world/` but is raylib-free and view-free: it never includes `ui/`, raylib or anything else of `scene/` (not `render/`, `WorldModel`, `WorldView`); it may use `input/` actions, injected as an `input::InputHandler`. Subfolders add no namespace, and the folder name is not the namespace (`world::`; do not confuse it with the generic `core::`).

## Systems

`system.hpp`: the `System` base class (`update(dt)`; a system gets the registry and bus in its constructor). Order in `WorldModel`'s list is order of execution:

1. `PlayerInputSystem(registry, input)`: sets the `Mover` direction of `PlayerControlled` entities from the held `Move*` actions (screen space turned into world space).
2. `RouteSystem(registry, bus, terrain, network)`: rebuilds the `RouteNetwork` on its first update (the buildings of a loaded save) and on the update after any `BuildingPlaced` (once per frame however many); never otherwise. `WorkerSystem(registry, bus, factory)`: spawns `HeadquartersWorkers` (100) on the headquarters when its `BuildingPlaced` arrives; each update an idle `Worker` takes the first construction site with room (`max_workers(site)` = effort / `EffortPerWorker` (10): a mine of 300 takes 30), becomes `Working`, sets its `Builder` site and follows the route headquarters -> site (`RouteEnd::Stop`); when the site is finished or gone it turns `Idle` and walks the route back (it can take a new site on the way). On `WorkerAssignmentRequested{building, count}` it sends `count` idle workers to a finished building with `Staffing` (capped at `needed` minus those already assigned; the job is `Worker::workplace`), a negative count sends that many back idle along the route; production jobs are never given automatically and are kept when sites appear. No headquarters = no jobs. `RouteFollowSystem(registry, network)`: steers every `RouteFollower` (see Pathfinding).
3. `CharacterMoveSystem(registry, bus)`: moves entities with `Character` + `Position` + `Mover` (normalized direction, so diagonals are not faster), sets the state and publishes `CharacterStateChanged{entity}` only when a character starts or stops moving.
4. `CameraFollowSystem(registry, camera)`: centres the camera on the `PlayerControlled` entity.
5. `BuildSystem`: builds on `BuildRequested`, paid by the player, and publishes `BuildingPlaced{tile}` (sites included); each placement is logged (`Placed <name> at (x, y)`). `ConstructionWorkSystem`: each `Builder` within `BuildReach` of its site adds `rate * dt`; at the effort the site is removed, its builders stop and `BuildingCompleted` is published.
6. `InventoryTransferSystem`: on `ItemTransferRequested{from, to, item, count}` runs `transfer_items()` when both have an `Inventory` and are within `TransferReach` (2 tiles; `within_transfer_reach()`).
7. `PowerSystem(registry)`: one global power network. Supply = `power_kw` of every finished `Generator` (one with an `input` burns `input.count` from its `Inventory` every 1/`rate` s and supplies nothing without it, keeping at most one burn pending); demand = `demand_kw` of every finished `PowerConsumer`. When demand > supply every consumer gets `powered = false`, otherwise `true` (equal is enough). Sets `Generator::running` (false while out of fuel). `supply_kw()` / `demand_kw()` of the last update. Generator progress and `powered` are not saved (recomputed next frame).
8. `ProductionSystem(registry, terrain)`: finished buildings with `Producer` + `Inventory` (and no unpowered `PowerConsumer`): progress += rate * dt, each whole unit makes one output into the building's inventory; waits with at most one unit banked while the input is missing or the output does not fit. Sets the `BuildingStatus` (when present) to `Working`, or `Idle` with the first reason: `NoPower`, `NoResource` (no output, a mine off ore), `NoWorkers`, `MissingInput`, `OutputFull`. A building with `Staffing` runs at `rate * min(present, needed) / needed`, where present = workers whose `workplace` is it and that stand within `BuildReach`; none present = `NoWorkers`. `item_from_tile()`. Production progress is not saved.

## Components

- `Character` (marks a character, holds its `CharacterState`: `Idle` or `Moving`), `Position` (a `WorldPoint`), `Mover` (speed in tiles/s + direction intent), `PlayerControlled`.
- `Inventory` (`inventory.hpp` + `.cpp`): a fixed number of `ItemStack` slots; `add()` returns what did not fit and tops up same-type stacks before using empty slots; `remove()` returns what it took, from the last slots first; `count()`, `free_slots()`. `transfer_items(from, to, item, count)` moves as much as the source holds and the target fits and returns how many moved.
- `building.hpp`: `Building { type, tile }`, `ConstructionSite { progress, effort }` (present while unfinished), `Producer { recipe, progress }` (its own copy of the recipe so the rate can change per building), `BuildingStatus { state, reason }` with `BuildingState` (`Working`, `Idle`) and `IdleReason` (`None`, `NoPower`, `NoResource`, `MissingInput`, `OutputFull`); set by the building's systems every frame, not saved.
- `power.hpp`: `Generator { spec, progress, running }`, `PowerConsumer { demand_kw, powered }` (only whether power arrives; whether the building works is its `BuildingStatus`).
- `power_report.hpp`: `power_report(registry)` -> `PowerReport { generators, consumers, produced_kw, demand_kw, overloaded() }`, each a `PowerEntry { id, name (entity label), kw, active }`; finished buildings only, as the PowerSystem left them.
- `builder.hpp`: `Builder { rate, site }` (`CharacterBuildRate` 10/s, `BuildReach` 2 tiles; every character has one).
- `worker.hpp`: `Worker { state, workplace }`, `Staffing { needed }` (workers for the full production rate) with `WorkerState` (`Idle`, `Working`), `EffortPerWorker`, `HeadquartersWorkers`, `max_workers()`. The job is the `Builder`'s site. Saved by `WorkerCodec` (state, workplace, site and walked route, by building tile); a loaded worker carries a `PendingWorkerLinks` until the `WorkerSystem`'s next update turns the tiles into ids (a building no longer there = idle).
- `worker_report.hpp`: `worker_report(registry)` -> `WorkerReport { idle, workplaces }`, each a `WorkplaceEntry { id, name, tile, assigned, needed }`; finished buildings with `Staffing` only, in registry order. `is_at_work(registry, worker)`: `Working` and within `BuildReach` of its workplace (or, without one, its `Builder` site); the ProductionSystem counts these as present and the `EntityRenderer` does not draw them (they are inside the building).
- `persistent.hpp`: `Persistent{archetype}`; only entities with it are saved.

## Pathfinding

`pathfinding/` computes walks between buildings once, not per frame:

- `pathfinder.hpp`: `find_path(start, goal, is_blocked, max_nodes = MaxPathSearchNodes)`, A* over 8 neighbours (diagonal cost sqrt(2), no corner cutting: a diagonal needs both orthogonal neighbours free). `start` and `goal` are always walkable (they are buildings). The world has no edge, so the search gives up (none) after `max_nodes` (20000) expanded tiles.
- `route_network.hpp`: `RouteKey{from, to}` (building ids, stable across rebuilds), `Route{key, tiles}` (from the `from` tile to the `to` tile, both included; empty = unreachable). `RoutePlanner` decides which pairs get a route: `HeadquartersRoutePlanner` = headquarters to every other building. A new combination = a new planner subclass. `RouteNetwork(planner)`: `rebuild(registry, terrain)` (blocked = water or any building tile, sites included), `find(key)`, `revision()` (grows each rebuild); each rebuild is logged with its revision, route count and unreachable count. Owned by `WorldModel` (`routes()`); not saved, recomputed after loading.
- `components/route_follower.hpp`: `RouteFollower{route, end, waypoint, forward, revision}` signs an entity with `Position` + `Mover` up to a route; `RouteEnd::Stop` stands still on the last tile, `RouteEnd::Loop` walks back and forth. `RouteFollowSystem` points the `Mover` at the next waypoint's centre and moves on when within one step; after a rebuild it rejoins at the nearest waypoint (ties go forward); no route or unreachable = stands still. Not for `PlayerControlled` entities. Not saved.

## Items

`items/item.hpp`: `ItemType` (`IronOre`, `CopperOre`, `Coal`), `Item { type, max_stack }` (definitions in `items::`, 100 each), `ItemStack { item, count }`, `to_string(ItemType)`.

## Entities

- `entities/character.hpp`: `spawn_character()` (`"character"` archetype): `Character` + `Position` + `Mover` at `CharacterSpeed` + an `Inventory` of `CharacterInventorySlots` (40) + a `Builder`. A new game's player gets `StartingIronOre` (200).
- `entities/worker.hpp`: `spawn_worker()` (`"worker"` archetype, `WorkerArchetype`): a character + `Worker`. Drawn as a cube like the player, labelled with its worker state.
- `entities/building.hpp`: `spawn_building()` (`"building"` archetype) and `make_building()`: `Building` + `Position` at the tile centre + `blueprint.apply()` + a site when effort > 0. It knows no building feature: those come from the blueprint.
- `entities/entity_factory.hpp`: `EntityFactory` (archetype name → spawn function, injected; `make_entity_factory()` registers them; `spawn()` adds `Persistent`). `WorldModel` spawns the new-game player through it too.

## Events

`events/character_state_changed.hpp` (`CharacterStateChanged`), `events/building_events.hpp` (`BuildRequested{type, tile}`, `BuildingPlaced{tile}`, `BuildingCompleted{entity}`), `events/item_transfer_requested.hpp` (`ItemTransferRequested`), `events/worker_events.hpp` (`WorkerAssignmentRequested`).

## Construction

- `construction/building_type.hpp`: `BuildingType` (`Headquarters`, `Mine`; one value per implemented building; `building_type_key()` / `parse_building_type()` for saves).
- `construction/blueprint.hpp`: `Blueprint`, the prefab of a building: `with(component)` (any component type, chainable), `apply(registry, id)` (adds a copy of each; one the entity already has is kept, so a loaded inventory survives), `find<C>()` (the blueprint's data of C or null). A new building feature = a component + a `.with()` in the catalog; `Construction` and `make_building()` do not change.
- `construction/build_rule.hpp`: `BuildResult`, `has_headquarters()`, the `BuildRule` interface (`check_unlocked(registry, type)` before a tile is chosen, `check_site(terrain, tile)` once the tile is free; both default Ok) and the rules `UniqueRule` (AlreadyExists), `NeedsHeadquartersRule` (NeedsHeadquarters), `NeedsOreRule` (NeedsOre). A new placement condition = a new subclass.
- `construction/construction.hpp`: `ConstructionCategory` (`Special`, `Mine`, `Factory`, `Military`); `Construction { name, icon, type, cost, effort, rules, blueprint }` where `type` empty = listed but not buildable yet, `cost` = item stacks paid from the player's inventory, `effort` = work units, 0 = finished at once, `rules` = its `BuildRule`s in order, `blueprint` = its components; `ConstructionGroup`; `construction_catalog()` (by category, in build bar order) and `find_construction(type)`. `Recipe { input, output, rate }` lives with `Producer` in `components/building.hpp` (every 1/`rate` s consumes the optional `input` stack and adds one `output`; empty `output` = the ore under the building); `GeneratorSpec { type, input, rate, power_kw }` / `GeneratorType` (`Solar`) live with `Generator` in `components/power.hpp`.
  - Headquarters: free, effort 0; rules: Unique (the only entry without NeedsHeadquarters); blueprint: solar `Generator`, 1000 kW.
  - Mine: 100 iron ore, effort 300; rules: NeedsHeadquarters, NeedsOre; blueprint: `Inventory` 10 slots, `Producer` (no input, the ore below, 0.5/s at 10 workers), `PowerConsumer` 100 kW, `BuildingStatus`, `Staffing` 10.
- `construction/build_rules.hpp`: `check_unlocked()` (NotBuildable, then each rule's `check_unlocked()`), `can_build()` (+ TileBlocked: water or another building, each rule's `check_site()`, NotEnoughResources), `build()` (applies them, pays the cost, spawns through the factory).

## Terrain

`terrain/terrain.hpp`: `Terrain(seed)`, a flat unbounded world of `TileType::Grass` / `Water` / `IronOre` / `CopperOre` / `Coal` (`is_ore()`). `tile_at(tile)` is a pure function of the seed (value noise), so a loaded save recreates the same world:

- the `SpawnClearRadius` around (0,0) is always grass;
- three medium starting ore nodes, one per type, 24–36 tiles out, each on the first dry spot around its seed-random angle (when none is fully dry, the driest one is used and its footprint turned into land), so ore never touches water;
- scattered nodes, one per 64-tile cell, only on land, none inside `StartingAreaRadius` (64), radius growing with the distance from (0,0) up to a cap; irregular edges via the noise.

## Saving entities

- `save/component_codec.hpp`: `ComponentCodec` (`write` / `read` of one component's `Fields`).
- `save/codecs.hpp`: `PositionCodec`, `CharacterCodec` (writes and restores `Moving`/`Running` as `Idle` through `saved_state()`), `PlayerControlledCodec`, `InventoryCodec` (`inventory.<slot>=<ItemType>:<count>` per filled slot, invalid entries skipped), `BuildingCodec` (`building.type`, `building.x`, `building.y`; re-places the building), `ConstructionSiteCodec` (`construction.progress`; missing = finished), `WorkerCodec` (`worker.state`, `worker.workplace.x/y`, `worker.site.x/y`, `worker.route.x/y` + `worker.route.forward`: buildings referenced by tile since ids are not saved, resolved after loading through `PendingWorkerLinks`).
- `save/world_serializer.hpp`: `WorldSerializer` (`snapshot()` / `restore()`); `make_world_serializer(factory)` lists the codecs: a new saved component = one codec there. `BuildingCodec` comes first because re-placing a building recreates its components. Entity ids are not saved. This is the one place where `world::` depends on `game/save_game.hpp` (plain data).

## Camera and isometric math

- `camera.hpp`: `Camera` (`center`, `projection(w, h)` puts the centre at the middle of the screen, `MinZoom`/`MaxZoom`). `TerrainRenderer` and `EntityRenderer` draw through its projection.
- `isometric.hpp` (2:1 diamond projection): `TileCoord`, `WorldPoint` (continuous, in tiles), `ScreenPoint`, `IsoProjection` (tile size + `origin`, the screen position of world (0,0): move it to scroll, scale the tile size to zoom) with `world_to_screen()`, `screen_to_world()`, `tile_at()` (mouse picking), `tile_center()`, `tile_corners()`; `tile_containing()` floors a world position to its tile; `depth_of()` / `draws_before()` give the back-to-front order; `screen_direction_to_world()` (screen up = world (-1,-1)).
