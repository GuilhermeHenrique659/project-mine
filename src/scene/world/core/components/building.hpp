#pragma once

#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// A building standing (or being built) on one tile.
struct Building {
    BuildingType type = BuildingType::Headquarters;
    TileCoord tile{};
};

// Present while a building is unfinished: work done so far and the work it needs.
struct ConstructionSite {
    float progress = 0.0f;
    float effort = 0.0f;
};

// A building that produces (see Recipe). It holds its own copy of the recipe so the rate
// can change per building; `progress` counts towards the next output (1 = one output).
struct Producer {
    Recipe recipe;
    float progress = 0.0f;
};

} // namespace world
