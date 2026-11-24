#pragma once

#include <vector>

#include "core/entity.hpp"

namespace world {

// The player orders units (the selection) into a vehicle: each walks to it and gets in
// while there is a free seat (see VehicleSystem).
struct UnitsBoardRequested {
    std::vector<core::EntityId> units;
    core::EntityId vehicle = 0;
};

// A character asks to get into a vehicle right away: done when it stands within BoardReach.
struct VehicleBoardRequested {
    core::EntityId passenger = 0;
    core::EntityId vehicle = 0;
};

// A passenger asks to get out: it appears on the nearest free tile beside the vehicle.
struct VehicleExitRequested {
    core::EntityId passenger = 0;
};

// The player asks every unit inside `vehicle` to get out (the HUD Unload button): each
// appears on the nearest free tile beside it. The player, if inside, stays.
struct VehicleUnloadRequested {
    core::EntityId vehicle = 0;
};

} // namespace world
