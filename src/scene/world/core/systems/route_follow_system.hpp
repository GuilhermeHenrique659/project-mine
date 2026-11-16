#pragma once

#include "core/registry.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Steers every RouteFollower: points its Mover at the next waypoint, LaneOffset right of
// the tile centre along its walking direction (two lanes, one per direction), and moves on
// to the next one within ArriveRadius or once past it (RouteEnd decides what happens at the
// end; a Stop end is reached exactly on the last tile's centre). When the
// network was rebuilt since the last frame, it joins the new route at the waypoint nearest
// to the entity. Without its route (or an unreachable one) the entity stands still.
// Runs before CharacterMoveSystem, which does the actual walking.
// How far right of a route's centre line a follower walks, in tiles: followers going
// opposite ways on the same route pass each other on two lanes.
inline constexpr float LaneOffset = 0.2f;

// Within this distance of a waypoint (or past it along the route) a follower moves on to
// the next one, so a follower pushed off its lane never circles a point it cannot reach.
inline constexpr float ArriveRadius = 0.3f;

class RouteFollowSystem : public System {
public:
    RouteFollowSystem(core::Registry& registry, const RouteNetwork& network);

    void update(float delta_time) override;

private:
    core::Registry& registry_;
    const RouteNetwork& network_;
};

} // namespace world
