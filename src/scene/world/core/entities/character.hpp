#pragma once

#include <cstddef>
#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

// Walking speed of a character, in tiles per second.
constexpr float CharacterSpeed = 4.0f;

// Number of inventory slots a character carries.
constexpr std::size_t CharacterInventorySlots = 40;

// Iron ore the player's character carries at the start of a new game.
constexpr int StartingIronOre = 200;
// The player starts a new game with a weapon and a full stack of ammo, so it can shoot.
constexpr int StartingWeapons = 1;
constexpr int StartingAmmo = items::Ammo.max_stack;

// A character is an archetype, not a class: an entity with the Character marker,
// Position, Mover, a SolidInventory of CharacterInventorySlots and a Builder.
core::EntityId spawn_character(core::Registry& registry, std::string label, WorldPoint position);

} // namespace world
