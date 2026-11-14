#pragma once

#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// Walking speed of a character, in tiles per second.
constexpr float CharacterSpeed = 4.0f;

// A character is an archetype, not a class: an entity with the Character marker,
// Position and Mover.
core::EntityId spawn_character(core::Registry& registry, std::string label, WorldPoint position);

} // namespace world
