#pragma once

#include <string>
#include <vector>

#include "core/registry.hpp"

namespace scene::world {

// The detail lines of a building, one per feature it has, in this order:
// "State: Working" / "State: Idle (no power)" (BuildingStatus), "Power: 100 kW"
// (PowerConsumer), "Generates: 1000 kW" (Generator). Empty for an entity with none.
std::vector<std::string> building_details(const core::Registry& registry, core::EntityId id);

} // namespace scene::world
