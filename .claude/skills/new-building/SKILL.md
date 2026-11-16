---
name: new-building
description: Checklist for adding a new building to this game (a factory, smelter, steelworks, generator, storage, mine variant...) - catalog entry, BuildingType, recipe, items, workers, power, inventory sections with item filters, saves, UI colors and docs. Use whenever the user asks to create, add or "make a new build/building", gives a building's cost, effort, workers, power or recipe, or asks to change those numbers on an existing building, even if they never say "building".
---

# Adding a new building

A building touches many small places spread across `scene/world/core/`. Missing one usually still compiles (with a `-Wswitch` warning) but silently breaks saves, worker labels or the UI. This checklist is the full list of those places, taken from how the Steelworks was added.

Work through the `tdd` skill: write the tests first, one behaviour at a time. This skill only says *what* to touch and test.

## 1. Pin down the spec

Restate the numbers the user gave and fill the gaps with an explicit assumption (say it in the reply so the user can correct it):

| Field | Where it lives | Usual default if not given |
|---|---|---|
| Name, catalog tab (Special / Mines / Factories / Military) | `construction_catalog()` | Factories for anything with a recipe |
| Cost (item stacks) and effort | `Construction{..., cost, effort, ...}` | — ask or assume iron ore |
| Footprint | `TileSize{w, h}` | 2x2 small, 3x3 for a big factory |
| Build rules | `NeedsHeadquartersRule`, `NeedsOreRule`, `UniqueRule` | NeedsHeadquarters |
| Recipe: inputs, output, rate (outputs per second) | `Producer{Recipe{inputs, output, rate}}` | — |
| Workers for full rate | `Staffing{n}` | — |
| Power draw (kW) | `PowerConsumer{kw}` | — |
| Inventory | `Inventory{general}.append(n, ItemType::X)` | 10 general slots |

Interpret loosely-worded items against the items that exist (`items/item.hpp`): "iron" means `IronOre` while there is no iron plate. If the recipe produces a new item, the item is part of the job (step 3).

"Rate of 2 per second" is the output rate: `rate = 2.0f`. The inputs are consumed per output, so 2 + 2 inputs at 2/s means 4 + 4 per second; mention this in the summary.

## 2. Inputs go in filtered inventory sections

A producer with inputs gets one filtered section per input (`Inventory::append(count, ItemType)`). An item with filtered slots only goes into them, so inputs can never fill the general slots and block the output, and Take-mode haulers only take from general slots (the output). Size each input section so it holds a few seconds of consumption; 5 slots (500 items) is the Steelworks choice.

Buildings without inputs (mines, storage) need no sections.

## 3. Places to touch

Each enum switch here has no `default:`, so the compiler's `-Wswitch` warning lists the ones you missed. Build and read the warnings: zero warnings is part of done.

1. **`construction/building_type.hpp`**: add the `BuildingType` value.
2. **`construction/construction.cpp`**: `building_type_key()` (stable save name, PascalCase, never rename later), `parse_building_type()`, and the catalog entry. If the catalog already lists the building as a placeholder (`{"Smelter", "icon_smelter"}`, no type), replace that placeholder instead of adding a second one. The icon name is `icon_<snake_name>`; assets live outside git, so tell the user the icon file is needed.
3. **`worker_type.hpp` / `.cpp`**: a production building (has `Staffing`) gets its own `WorkerType` (Mine -> Miner, Steelworks -> Steelworker): enum value, `to_string()`, and a case in `type_at()`.
4. **New item** (only if the output or an input is new): `items/item.hpp` (`ItemType`, `items::X` with `max_stack` 100, `item_of()`, `to_string()`), `save/codecs.cpp` `ItemKeys` (bump the array size), `render/inventory_grid.cpp` `item_color()` (non-tile colors are local constants there; ore colors come from `terrain_renderer.hpp`).
5. **Production rules**: if the building needs something `ProductionSystem` or `Recipe` cannot express yet (several inputs, a new idle reason, fuel...), generalise the system instead of special-casing the building, and update the existing tests that construct `Recipe` in the same step.
6. **Docs**: `docs/gameplay.md` (build bar list: what the player sees) and `docs/world-logic.md` (catalog entry line, and any enum list you extended: `ItemType`, `WorkerType`, `BuildingType`).

## 4. Tests that earn their place

Follow the existing style in these files (Given / When / Then, PascalCase names that state the numbers):

- `tests/scene/world/core/construction/construction_test.cpp`
  - the catalog entry: cost, effort, staffing, power (`SteelworksCosts300IronOreTakes1200WorkNeeds50WorkersOn200Kw`);
  - the recipe: inputs, output, rate;
  - the inventory layout: fill it with each input and the output and check the leftovers, which proves the sections and the general slots;
  - the save key round trip (`BuildingType, <Name>RoundTripsThroughItsSaveKey`).
- `tests/scene/world/core/systems/production_system_test.cpp`: a finished, fully staffed instance built with `make_building()` from the catalog entry, given its inputs, produces the expected output and consumes the expected inputs over one second.
- `tests/scene/world/core/worker_type_test.cpp`: a worker assigned to it has the new worker type and its label.
- `tests/scene/world/core/save/world_serializer_test.cpp`: a new item survives a save and load.

Get the red with stubs only (the new enum values, nothing else): the tests then fail on their assertions (`find_construction` returns null, key is empty, label is "Idle"). If a test passes before implementation, say so in the report instead of hiding it.

## 5. Finish

Build, run the full suite (`ctest --test-dir build --output-on-failure`), confirm zero warnings, then report with the `tdd` summary plus:

- the assumptions you made (footprint, inventory sizes, item interpretation);
- the throughput (inputs consumed per second at full crew);
- what the user still has to do (icon asset) and known gaps (e.g. Put-mode haulers bring whatever the storage holds first, not the building's inputs).

Do not commit unless asked.
