#pragma once

#include <cstddef>
#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// Archetype name of a soldier in the entity factory and in saves.
inline constexpr const char* SoldierArchetype = "soldier";

// Slots of a soldier's inventory.
inline constexpr std::size_t SoldierInventorySlots = 2;

// A soldier is a unit (see spawn_unit) with the Soldier marker and an inventory of
// SoldierInventorySlots.
core::EntityId spawn_soldier(core::Registry& registry, std::string label, WorldPoint position);

} // namespace world
