#pragma once

#include "core/entity.hpp"

namespace world {

// The player asks to send `count` idle workers to a production building (negative =
// send that many of its workers back). The WorkerSystem applies what it can: never more
// than the building's Staffing needs, nor more idle workers than there are.
struct WorkerAssignmentRequested {
    core::EntityId building = 0;
    int count = 0;
};

} // namespace world
