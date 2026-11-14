#pragma once

#include "world/isometric.hpp"

namespace world {

// Lets an entity walk. `speed` is in tiles per second; `direction` is the current
// movement intent in world space ((0, 0) = standing still). Input writes it, the move
// system reads it.
struct Mover {
    float speed = 0.0f;
    WorldPoint direction{};
};

} // namespace world
