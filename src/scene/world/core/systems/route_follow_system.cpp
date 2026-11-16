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

// The unit direction a follower walks into waypoint `i` with: from the waypoint before it
// (along its walking direction), or towards the one after it at the start of the route.
WorldPoint heading_into(const Route& route, std::size_t i, bool forward) {
    const auto& tiles = route.tiles;
    const bool has_previous = forward ? i > 0 : i + 1 < tiles.size();
    const bool has_next = forward ? i + 1 < tiles.size() : i > 0;
    TileCoord from = tiles[i];
    TileCoord to = tiles[i];
    if (has_previous) {
        from = tiles[forward ? i - 1 : i + 1];
    } else if (has_next) {
        to = tiles[forward ? i + 1 : i - 1];
    }
    const float dx = static_cast<float>(to.x - from.x);
    const float dy = static_cast<float>(to.y - from.y);
    const float length = std::hypot(dx, dy);
    return length > 0.0f ? WorldPoint{dx / length, dy / length} : WorldPoint{};
}

// Whether waypoint `i` is where a Stop follower stands still: it ends there on the centre.
bool is_final(const RouteFollower& follower, std::size_t i, std::size_t size) {
    return follower.end == RouteEnd::Stop && (follower.forward ? i + 1 == size : i == 0);
}

// Where the follower aims for waypoint `i`: the tile centre moved LaneOffset to the right
// of its heading (so the two directions use two lanes), the plain centre for a Stop end.
WorldPoint target_of(const Route& route, const RouteFollower& follower, std::size_t i) {
    const WorldPoint centre = centre_of(route.tiles[i]);
    if (is_final(follower, i, route.tiles.size())) {
        return centre;
    }
    const WorldPoint heading = heading_into(route, i, follower.forward);
    // Right of the heading, in world coordinates (y grows downwards on screen).
    return {centre.x - heading.y * LaneOffset, centre.y + heading.x * LaneOffset};
}

// Whether the follower is done with waypoint `i`: close enough, or already past it.
bool arrived(const Route& route, const RouteFollower& follower, std::size_t i, WorldPoint at,
             WorldPoint target, float step) {
    if (is_final(follower, i, route.tiles.size())) {
        return distance(at, target) <= step;
    }
    if (distance(at, target) <= std::max(step, ArriveRadius)) {
        return true;
    }
    const WorldPoint heading = heading_into(route, i, follower.forward);
    return (target.x - at.x) * heading.x + (target.y - at.y) * heading.y <= 0.0f;
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
            WorldPoint target = target_of(*route, follower, follower.waypoint);
            if (arrived(*route, follower, follower.waypoint, position.point, target, step)) {
                // Only a Stop end is stood on exactly; lane points are passed through.
                if (is_final(follower, follower.waypoint, route->tiles.size())) {
                    position.point = target;
                }
                if (!advance(follower, route->tiles.size())) {
                    return;
                }
                target = target_of(*route, follower, follower.waypoint);
            }
            mover.direction = {target.x - position.point.x, target.y - position.point.y};
        });
}

} // namespace world
