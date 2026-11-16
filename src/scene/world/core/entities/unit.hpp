#pragma once

#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// A unit is a character (see spawn_character) with the Unit marker: the base every unit
// archetype builds on (spawn_worker() adds a Worker on top of it).
core::EntityId spawn_unit(core::Registry& registry, std::string label, WorldPoint position);

} // namespace world
