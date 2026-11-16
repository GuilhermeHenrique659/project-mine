#pragma once

#include "core/entity.hpp"
#include "scene/world/core/components/hauler.hpp"

namespace world {

// The player asks to send `count` idle workers to a production building (negative =
// send that many of its workers back). The WorkerSystem applies what it can: never more
// than the building's Staffing needs, nor more idle workers than there are.
struct WorkerAssignmentRequested {
    core::EntityId building = 0;
    int count = 0;
};

// The player asks to make `count` idle workers haulers of a logistic route in `mode`
// (negative = send that many of its haulers back idle). See HaulSystem.
struct HaulersRequested {
    RouteKey route;
    HaulMode mode = HaulMode::Take;
    int count = 0;
};

// The player switches every hauler of a logistic route to `mode`.
struct HaulModeRequested {
    RouteKey route;
    HaulMode mode = HaulMode::Take;
};

} // namespace world
