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

} // namespace world
