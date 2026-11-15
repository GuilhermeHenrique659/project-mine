#pragma once

#include "core/entity.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// The player asked to place a building on a tile (the build system checks the rules).
struct BuildRequested {
    BuildingType type = BuildingType::Headquarters;
    TileCoord tile{};
};

// Published once when a construction site reaches its effort and becomes a finished
// building (a building with no effort is finished at once and publishes nothing).
struct BuildingCompleted {
    core::EntityId entity = 0;
};

} // namespace world
