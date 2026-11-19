#pragma once

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// How many rings of tiles around a building are searched for a spawn tile.
inline constexpr int MaxSpawnRing = 8;

// Where a unit coming out of `building` appears: the nearest tile around its footprint
// (ring by ring, up to MaxSpawnRing) with no building, no water and no unit on it; none
// when every one is taken.
std::optional<TileCoord> spawn_tile(const core::Registry& registry, const Terrain& terrain,
                                    const Building& building);

} // namespace world
