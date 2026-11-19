#pragma once

#include <cstddef>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/pathfinding/pathfinder.hpp"

namespace world {

// The soldiers whose position, projected on screen, lies inside the rectangle spanned by
// the two corners `a` and `b` (in any order, edges included). Only soldiers can be selected.
std::vector<core::EntityId> soldiers_in(const core::Registry& registry,
                                        const IsoProjection& projection, ScreenPoint a,
                                        ScreenPoint b);

// `count` distinct walkable tiles to send a group to, nearest to `goal` first (`goal`
// itself when it is free), so units ordered together do not all stand on one tile. Fewer
// when no free tile is found within MaxFormationRadius tiles of `goal`.
inline constexpr int MaxFormationRadius = 8;
std::vector<TileCoord> formation_tiles(TileCoord goal, std::size_t count,
                                       const BlockedFn& is_blocked);

} // namespace world
