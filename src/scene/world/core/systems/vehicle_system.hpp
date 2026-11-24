#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Getting in and out of vehicles. VehicleBoardRequested boards a character within
// BoardReach; VehicleExitRequested puts a passenger on the nearest free tile beside its
// vehicle, and VehicleUnloadRequested does that for every passenger but the player.
// UnitsBoardRequested gives each unit a BoardOrder: update() walks it to the
// vehicle (UnitsApproachRequested, re-sent only when the vehicle is on another tile) and
// boards it once within BoardReach (a full vehicle drops the order); UnitsMoveRequested
// drops it too. update() then keeps every passenger on its vehicle's position, standing
// still, and lets out the passengers of a vehicle that is gone. Run it after the
// CharacterMoveSystem so passengers are where their vehicle moved this tick.
class VehicleSystem : public System {
public:
    VehicleSystem(core::Registry& registry, event::EventBus& bus, const Terrain& terrain);

    void update(float delta_time) override;

private:
    void exit(core::EntityId passenger);
    bool within_reach(core::EntityId vehicle, core::EntityId passenger) const;

    core::Registry& registry_;
    event::EventBus& bus_;
    const Terrain& terrain_;
    event::ScopedSubscription board_subscription_;
    event::ScopedSubscription exit_subscription_;
    event::ScopedSubscription order_subscription_;
    event::ScopedSubscription move_subscription_;
    event::ScopedSubscription unload_subscription_;
};

} // namespace world
