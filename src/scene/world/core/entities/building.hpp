#pragma once

#include <string>

#include "core/registry.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// Archetype name of every building in the entity factory and in saves.
inline constexpr const char* BuildingArchetype = "building";

// The "building" archetype: a Building (a headquarters on tile (0, 0) until placed) and
// its Position, until made a real one with make_building().
core::EntityId spawn_building(core::Registry& registry, std::string label);

// Makes the entity the given building of `construction` (which must have a type) on
// `tile`: Building, Position at the tile centre, and a ConstructionSite when its effort is
// above zero (none otherwise), plus an Inventory of its `inventory_slots` when it has any
// (an existing one of that size is kept) and a Producer of its recipe when it has one.
void make_building(core::Registry& registry, core::EntityId id, const Construction& construction,
                   TileCoord tile);

} // namespace world
