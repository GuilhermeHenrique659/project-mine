#pragma once

#include "core/entity.hpp"
#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// What can be shot on `tile`: the building covering it, or else the first unit or character
// standing on it, as long as it has Health and is not `exclude` (the shooter). 0 = nothing.
core::EntityId shootable_at(const core::Registry& registry, TileCoord tile, core::EntityId exclude);

} // namespace world
