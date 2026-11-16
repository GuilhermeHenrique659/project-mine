#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "scene/world/core/isometric.hpp"

namespace world {

// Whether a tile cannot be walked on (water, a building).
using BlockedFn = std::function<bool(TileCoord)>;

// How many tiles A* may expand before giving up. The world has no edge, so without a
// limit an unreachable goal would make the search run forever.
inline constexpr int MaxPathSearchNodes = 20000;

// Shortest walkable path from `start` to `goal`, both included, with A* over 8 neighbours
// (a diagonal step costs sqrt(2) and is only allowed when both orthogonal neighbours are
// free, so a path never squeezes past a corner). `start` and `goal` are always walkable,
// even when blocked: they are usually buildings. None when no path is found within
// `max_nodes` expanded tiles.
std::optional<std::vector<TileCoord>> find_path(TileCoord start, TileCoord goal,
                                                const BlockedFn& is_blocked,
                                                int max_nodes = MaxPathSearchNodes);

} // namespace world
