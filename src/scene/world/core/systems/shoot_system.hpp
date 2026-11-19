#pragma once

#include <functional>
#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/items/item.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// The stats of the first weapon in the inventory; empty when it holds none.
std::optional<WeaponStats> held_weapon(const Inventory& inventory);

// A dice roll for a shot: a uniform number in [0, 1). The shot hits when it is below the
// hit chance.
using HitRoll = std::function<float()>;

// A HitRoll backed by a randomly seeded generator.
HitRoll random_hit_roll();

// How much each point of experience adds to the hit chance (0.5%: 80 shots of experience
// take a 60% weapon to 100%).
inline constexpr float AccuracyPerExperience = 0.005f;

// The chance (0 to 1) a shot hits: the weapon's accuracy plus AccuracyPerExperience per
// point of the shooter's experience, kept within 0 and 1.
float hit_chance(const WeaponStats& weapon, int experience);

// Shooting: on ShotRequested, a Shooter whose inventory holds a weapon item (see
// weapon_stats) and at least one Ammo fires at a live target within the weapon's range,
// when its cooldown is over. A shot spends one Ammo, restarts the cooldown, adds one
// experience and publishes ShotFired; it hits when `roll` comes below hit_chance(), and
// only then takes the weapon's damage from the target's Health (destroying it at 0).
// UnitsAttackRequested gives each unit an AttackOrder: update() then fires it at the target
// whenever it can, and drops the order once the target is gone or the ammo runs out;
// UnitsMoveRequested drops it too. update() also counts the cooldowns down. Everything passed must outlive it.
class ShootSystem : public System {
public:
    ShootSystem(core::Registry& registry, event::EventBus& bus,
                HitRoll roll = random_hit_roll());

    void update(float delta_time) override;

private:
    void shoot(core::EntityId shooter, core::EntityId target);

    core::Registry& registry_;
    event::EventBus& bus_;
    HitRoll roll_;
    event::ScopedSubscription subscription_;
    event::ScopedSubscription attack_subscription_;
    event::ScopedSubscription move_subscription_;
};

} // namespace world
