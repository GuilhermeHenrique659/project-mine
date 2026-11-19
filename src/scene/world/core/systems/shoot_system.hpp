#pragma once

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/items/item.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// The stats of the first weapon in the inventory; empty when it holds none.
std::optional<WeaponStats> held_weapon(const Inventory& inventory);

// Shooting: on ShotRequested, a Shooter whose inventory holds a weapon item (see
// weapon_stats) and at least one Ammo fires at a live target within the weapon's range,
// when its cooldown is over. A shot spends one Ammo, restarts the cooldown, takes the
// weapon's damage from the target's Health (destroying it at 0) and publishes ShotFired.
// UnitsAttackRequested gives each unit an AttackOrder: update() then fires it at the target
// whenever it can, and drops the order once the target is gone or the ammo runs out;
// UnitsMoveRequested drops it too. update() also counts the cooldowns down. Everything passed must outlive it.
class ShootSystem : public System {
public:
    ShootSystem(core::Registry& registry, event::EventBus& bus);

    void update(float delta_time) override;

private:
    void shoot(core::EntityId shooter, core::EntityId target);

    core::Registry& registry_;
    event::EventBus& bus_;
    event::ScopedSubscription subscription_;
    event::ScopedSubscription attack_subscription_;
    event::ScopedSubscription move_subscription_;
};

} // namespace world
