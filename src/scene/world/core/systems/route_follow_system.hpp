#pragma once

#include "core/registry.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Steers every RouteFollower: points its Mover at the centre of the next waypoint and,
// once there, moves on to the next one (RouteEnd decides what happens at the end). When the
// network was rebuilt since the last frame, it joins the new route at the waypoint nearest
// to the entity. Without its route (or an unreachable one) the entity stands still.
// Runs before CharacterMoveSystem, which does the actual walking.
class RouteFollowSystem : public System {
public:
    RouteFollowSystem(core::Registry& registry, const RouteNetwork& network);

    void update(float delta_time) override;

private:
    core::Registry& registry_;
    const RouteNetwork& network_;
};

} // namespace world
