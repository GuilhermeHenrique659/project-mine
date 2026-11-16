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
// above zero (none otherwise), plus the components of its blueprint (one the entity
// already has is kept, so a loaded inventory survives).
void make_building(core::Registry& registry, core::EntityId id, const Construction& construction,
                   TileCoord tile);

} // namespace world
