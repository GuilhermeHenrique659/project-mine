#pragma once

#include <cstddef>
#include <vector>

#include "scene/world/core/isometric.hpp"

namespace world {

// A path of its own an entity (with Position + Mover) walks, tile by tile, computed once
// by the pathfinder when it was ordered somewhere (see MoveOrderSystem). The
// PathFollowSystem steers its Mover to each tile centre in turn and removes the component
// on the last one. Not saved.
struct PathFollower {
    std::vector<TileCoord> tiles;
    // Index of the tile being walked to.
    std::size_t waypoint = 0;
};

} // namespace world
