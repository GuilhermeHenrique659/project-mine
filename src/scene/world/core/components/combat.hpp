#pragma once

#include "core/entity.hpp"

namespace world {

// Hit points a soldier or the player character starts with.
inline constexpr int MaxHealth = 100;

// An entity that can fire the weapon in its inventory (see ShootSystem). `cooldown` is the
// seconds left before it can fire again; `experience` counts the shots it has fired and
// raises its hit chance (see hit_chance). Not saved.
struct Shooter {
    float cooldown = 0.0f;
    int experience = 0;
};

// Hit points of an entity that can be shot; it is destroyed when they reach 0. Not saved
// (a loaded entity starts at full health).
struct Health {
    int current = MaxHealth;
};

// A standing order to shoot `target` whenever the shooter can, kept until the target is
// gone, the shooter runs out of ammo or gets a move order (see ShootSystem). Not saved.
struct AttackOrder {
    core::EntityId target = 0;
};

// Marks something to practise shooting on (the Training Target building): shots damage its
// Health as usual, but at 0 it is restored to MaxHealth instead of being destroyed.
struct TrainingTarget {};

} // namespace world
