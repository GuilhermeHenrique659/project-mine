#pragma once

#include <cstddef>
#include <cstdint>

#include "scene/world/core/pathfinding/route_network.hpp"

namespace world {

// What a follower does at the end of its route. A new behaviour = a new value handled in
// RouteFollowSystem's advance step.
enum class RouteEnd {
    Stop, // stands still on the last tile
    Loop, // walks back and forth between the two ends forever
};

// Signs an entity (with Position + Mover) up to a precomputed route of the RouteNetwork:
// the RouteFollowSystem steers its Mover along the waypoints, nothing is computed per
// frame. Do not give it to a PlayerControlled entity (input would fight it). Not saved.
struct RouteFollower {
    RouteKey route;
    RouteEnd end = RouteEnd::Stop;
    // Index of the waypoint being walked to, and the walking direction along the route.
    std::size_t waypoint = 0;
    bool forward = true;
    // The network revision the waypoint index belongs to (0 = not joined yet).
    std::uint64_t revision = 0;
};

} // namespace world
