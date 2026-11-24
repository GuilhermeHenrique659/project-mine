#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "core/entity.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

// How close (tiles, straight line) a character must be to a vehicle to get in.
inline constexpr float BoardReach = 2.0f;

// Something characters ride in (see spawn_vehicle): `seats` passengers at most, listed in
// boarding order. A vehicle moves and shoots only while someone is inside (is_crewed).
// The passengers are not saved: a loaded vehicle is empty.
struct Vehicle {
    std::size_t seats = 0;
    std::vector<core::EntityId> passengers;
};

// A character sitting in `vehicle`: it is not drawn, does not walk, collide, shoot or get
// shot, and rides along on the vehicle's position (see VehicleSystem). Not saved.
struct Passenger {
    core::EntityId vehicle = 0;
};

// A unit walking to `vehicle` to get in (UnitsBoardRequested, see VehicleSystem).
// `approach_goal` is the tile last asked for, so the walk is re-planned only when the
// vehicle has moved to another tile. Not saved.
struct BoardOrder {
    core::EntityId vehicle = 0;
    std::optional<TileCoord> approach_goal;
};

// A weapon built into its entity (a tank's cannon): it fires without a weapon item, still
// spending Ammo from the entity's inventory (see weapon_of).
struct MountedWeapon {
    WeaponStats stats;
};

// Marks a Battle Tank (see spawn_battle_tank).
struct BattleTank {};

} // namespace world
