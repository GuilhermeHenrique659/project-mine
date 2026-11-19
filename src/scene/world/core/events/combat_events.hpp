#pragma once

#include <vector>

#include "core/entity.hpp"

namespace world {

// Someone asks `shooter` to fire at `target`. The ShootSystem fires when the shooter holds
// a weapon and ammo, is off cooldown and the target is within the weapon's range.
struct ShotRequested {
    core::EntityId shooter = 0;
    core::EntityId target = 0;
};

// The player orders units (the selection) to keep shooting `target` until it is destroyed:
// the ShootSystem gives each an AttackOrder.
struct UnitsAttackRequested {
    std::vector<core::EntityId> units;
    core::EntityId target = 0;
};

// A shot was fired (published by the ShootSystem after the ammo is spent), e.g. for effects.
struct ShotFired {
    core::EntityId shooter = 0;
    core::EntityId target = 0;
};

} // namespace world
