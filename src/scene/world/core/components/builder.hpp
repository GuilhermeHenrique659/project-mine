#pragma once

#include "core/entity.hpp"

namespace world {

// How close (in tiles, centre to centre) a builder must be to work on a construction site.
inline constexpr float BuildReach = 2.0f;

// Effort a character puts into a construction site per second.
inline constexpr float CharacterBuildRate = 10.0f;

// An entity that can work on construction sites: `rate` effort units per second on
// `site` (0 = not working). The site is set by whoever drives the builder (the player's
// input) and cleared when the site is finished.
struct Builder {
    float rate = CharacterBuildRate;
    core::EntityId site = 0;
};

} // namespace world
