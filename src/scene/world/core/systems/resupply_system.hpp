#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// How close (tiles from the footprint edge) a soldier must be to take from a supply.
inline constexpr float SupplyReach = 1.0f;

// Resupplying soldiers at a Supply building (the Barracks).
// On UnitsResupplyRequested, each soldier gets a ResupplyOrder and a path (around water and
// buildings) to a free tile next to the building; UnitsMoveRequested drops the order.
// update(): a soldier with an order that is within SupplyReach takes one Weapon (only when
// it holds none) and tops its Ammo up to one full stack, from what the supply holds, then
// drops the order; one that stopped out of reach (no path) drops it too. Everything passed
// must outlive it.
class ResupplySystem : public System {
public:
    ResupplySystem(core::Registry& registry, event::EventBus& bus, const Terrain& terrain);

    void update(float delta_time) override;

private:
    void order(const UnitsResupplyRequested& request);

    core::Registry& registry_;
    const Terrain& terrain_;
    event::ScopedSubscription subscription_;
    event::ScopedSubscription move_subscription_;
};

} // namespace world
