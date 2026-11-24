#pragma once

#include <cstddef>
#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// What sets one kind of vehicle apart from another.
struct VehicleSpec {
    std::size_t seats = 1;
    float speed = 1.0f;
    int health = 100;
    std::size_t inventory_slots = 0;
};

// The base of every vehicle archetype ("abstract": it is never spawned on its own, see
// spawn_battle_tank): a unit (see spawn_unit, so it moves, collides and takes orders like
// one) that carries passengers instead of building: a Vehicle of `spec.seats`, Mover at
// `spec.speed`, Health of `spec.health` and a SolidInventory of `spec.inventory_slots`.
core::EntityId spawn_vehicle(core::Registry& registry, std::string label, WorldPoint position,
                             const VehicleSpec& spec);

// Puts `passenger` in a free seat of `vehicle`, dropping whatever it was ordered to do and
// its selection. False (nothing changes) when `vehicle` is not a vehicle, is full, is the
// passenger itself, or the passenger is a vehicle or already inside one.
bool board(core::Registry& registry, core::EntityId vehicle, core::EntityId passenger);

// Takes `passenger` out of its vehicle (it stays on the vehicle's position); nothing for a
// character that is not inside one.
void leave(core::Registry& registry, core::EntityId passenger);

// Whether someone sits in the vehicle: an empty one neither moves nor shoots.
bool is_crewed(const core::Registry& registry, core::EntityId vehicle);

// Whether the player character sits in the vehicle (then the player drives it).
bool carries_player(const core::Registry& registry, core::EntityId vehicle);

// The first vehicle standing on `tile`, 0 when none.
core::EntityId vehicle_at(const core::Registry& registry, TileCoord tile);

} // namespace world
