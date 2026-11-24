#pragma once

#include <cstddef>
#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

// Archetype name of a battle tank in the entity factory and in saves.
inline constexpr const char* BattleTankArchetype = "battle_tank";

inline constexpr std::size_t BattleTankSeats = 6;
inline constexpr float BattleTankSpeed = 12.0f;
inline constexpr int BattleTankHealth = 400;
inline constexpr std::size_t BattleTankInventorySlots = 10;
// The tank's cannon: 8 tiles, 50 damage, one shell every 2 s, semi-automatic, 60%.
inline constexpr WeaponStats TankCannon{8.0f, 50, 2.0f, FireMode::SemiAutomatic, 0.6f};

// A battle tank: a vehicle (see spawn_vehicle) of BattleTankSeats seats, with the
// BattleTank marker, a Shooter and the TankCannon mounted (it fires with the Ammo in its
// own inventory, while crewed).
core::EntityId spawn_battle_tank(core::Registry& registry, std::string label, WorldPoint position);

} // namespace world
