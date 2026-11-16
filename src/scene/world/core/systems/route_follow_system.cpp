#include "scene/world/core/systems/route_follow_system.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"

namespace world {

namespace {

// Closer than this to a waypoint counts as being on it even when standing still.
constexpr float ArriveEpsilon = 0.001f;

WorldPoint centre_of(TileCoord tile) { return {tile.x + 0.5f, tile.y + 0.5f}; }

float distance(WorldPoint a, WorldPoint b) { return std::hypot(b.x - a.x, b.y - a.y); }

// The waypoint nearest to `at`; a tie goes to the one further along the walking direction,
// so a rejoining follower never turns back.
std::size_t nearest_waypoint(const Route& route, WorldPoint at, bool forward) {
    std::size_t nearest = 0;
    float best = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < route.tiles.size(); ++i) {
        const float d = distance(at, centre_of(route.tiles[i]));
        if (d < best || (forward && d <= best)) {
            best = d;
            nearest = i;
        }
    }
    return nearest;
}

// Moves the follower to its next waypoint; false when it has nowhere left to go.
bool advance(RouteFollower& follower, std::size_t size) {
    if (size < 2) {
        return false;
    }
    if (follower.forward && follower.waypoint + 1 < size) {
        ++follower.waypoint;
        return true;
    }
    if (!follower.forward && follower.waypoint > 0) {
        --follower.waypoint;
        return true;
    }
    if (follower.end == RouteEnd::Stop) {
        return false;
    }
    follower.forward = !follower.forward;
    return advance(follower, size);
}

} // namespace

RouteFollowSystem::RouteFollowSystem(core::Registry& registry, const RouteNetwork& network)
    : registry_(registry), network_(network) {}

void RouteFollowSystem::update(float delta_time) {
    registry_.each<RouteFollower, Position, Mover>(
        [&](core::EntityId, RouteFollower& follower, Position& position, Mover& mover) {
            mover.direction = {};
            const Route* route = network_.find(follower.route);
            if (route == nullptr || !route->reachable()) {
                return;
            }
            if (follower.revision != network_.revision()) {
                follower.revision = network_.revision();
                follower.waypoint = nearest_waypoint(*route, position.point, follower.forward);
            }
            follower.waypoint = std::min(follower.waypoint, route->tiles.size() - 1);

            const float speed = mover.running ? mover.speed * RunSpeedMultiplier : mover.speed;
            const float step = std::max(speed * delta_time, ArriveEpsilon);
            WorldPoint target = centre_of(route->tiles[follower.waypoint]);
            if (distance(position.point, target) <= step) {
                position.point = target;
                if (!advance(follower, route->tiles.size())) {
                    return;
                }
                target = centre_of(route->tiles[follower.waypoint]);
            }
            mover.direction = {target.x - position.point.x, target.y - position.point.y};
        });
}

} // namespace world
