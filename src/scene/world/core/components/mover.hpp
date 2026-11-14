#pragma once

#include "scene/world/core/isometric.hpp"

namespace world {

// Lets an entity walk. `speed` is in tiles per second; `direction` is the current
// movement intent in world space ((0, 0) = standing still). Input writes it, the move
// system reads it.
struct Mover {
    float speed = 0.0f;
    WorldPoint direction{};
    bool running = false;
};

// How much faster a running mover is than a walking one.
constexpr float RunSpeedMultiplier = 2.0f;

} // namespace world
