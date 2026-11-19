#include "scene/world/core/systems/path_follow_system.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/components/position.hpp"

namespace world {

namespace {

// Closer than this to a tile centre counts as being on it even when standing still.
constexpr float ArriveEpsilon = 0.001f;

WorldPoint centre_of(TileCoord tile) { return {tile.x + 0.5f, tile.y + 0.5f}; }

} // namespace

void PathFollowSystem::update(float delta_time) {
    std::vector<core::EntityId> arrived;
    registry_.each<PathFollower, Position, Mover>(
        [&](core::EntityId id, PathFollower& follower, Position& position, Mover& mover) {
            mover.direction = {};
            const float speed = mover.running ? mover.speed * RunSpeedMultiplier : mover.speed;
            const float step = std::max(speed * delta_time, ArriveEpsilon);
            while (follower.waypoint < follower.tiles.size()) {
                const WorldPoint target = centre_of(follower.tiles[follower.waypoint]);
                const float dx = target.x - position.point.x;
                const float dy = target.y - position.point.y;
                const bool last = follower.waypoint + 1 == follower.tiles.size();
                if (std::hypot(dx, dy) > step) {
                    mover.direction = {dx, dy};
                    return;
                }
                if (last) {
                    position.point = target;
                }
                ++follower.waypoint;
            }
            arrived.push_back(id);
        });
    for (const core::EntityId id : arrived) {
        registry_.remove<PathFollower>(id);
    }
}

} // namespace world
