#include "scene/world/core/systems/character_move_system.hpp"

#include <cmath>
#include <vector>

#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/events/character_state_changed.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

namespace {

bool tile_blocked(const core::Registry& registry, const Terrain& terrain, TileCoord tile,
                  RouteKey passable) {
    if (terrain.tile_at(tile) == TileType::Water) {
        return true;
    }
    const core::EntityId building = building_at(registry, tile);
    return building != 0 && building != passable.from && building != passable.to;
}

// Whether a body of PlayerRadius at `point` overlaps water or a building other than the
// two `passable` ones.
bool blocked(const core::Registry& registry, const Terrain& terrain, WorldPoint point,
             RouteKey passable) {
    for (const float dx : {-PlayerRadius, PlayerRadius}) {
        for (const float dy : {-PlayerRadius, PlayerRadius}) {
            if (tile_blocked(registry, terrain, tile_containing({point.x + dx, point.y + dy}),
                             passable)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

bool CharacterMoveSystem::collides(core::EntityId id) const {
    if (registry_.has<PlayerControlled>(id)) {
        return true;
    }
    // A unit stuck too long walks through as a ghost (see UnitTraffic).
    const UnitTraffic* traffic = registry_.get<UnitTraffic>(id);
    return registry_.has<Unit>(id) && (traffic == nullptr || !traffic->ghost);
}

void CharacterMoveSystem::move(core::EntityId id, Position& position, WorldPoint step) const {
    const WorldPoint from = position.point;
    const WorldPoint to{from.x + step.x, from.y + step.y};
    // A unit goes into the buildings its route links (it works or unloads there).
    const RouteFollower* follower = registry_.get<RouteFollower>(id);
    const RouteKey passable = follower != nullptr ? follower->route : RouteKey{};
    // A body already overlapping an obstacle (a building placed over it) moves freely so
    // it can walk out.
    if (terrain_ == nullptr || !collides(id) || !blocked(registry_, *terrain_, to, passable) ||
        blocked(registry_, *terrain_, from, passable)) {
        position.point = to;
        return;
    }
    // Each axis on its own goes as far as it can, so the body ends touching the obstacle
    // and a diagonal step slides along it.
    WorldPoint at = from;
    at.x = furthest(at, {step.x, 0.0f}, passable).x;
    at.y = furthest(at, {0.0f, step.y}, passable).y;
    position.point = at;
}

WorldPoint CharacterMoveSystem::furthest(WorldPoint from, WorldPoint step,
                                         RouteKey passable) const {
    const auto along = [from, step](float fraction) {
        return WorldPoint{from.x + step.x * fraction, from.y + step.y * fraction};
    };
    if (!blocked(registry_, *terrain_, along(1.0f), passable)) {
        return along(1.0f);
    }
    // Binary search for the largest free fraction of the step.
    float free = 0.0f;
    float hit = 1.0f;
    for (int i = 0; i < ContactSearchSteps; ++i) {
        const float middle = (free + hit) / 2.0f;
        if (blocked(registry_, *terrain_, along(middle), passable)) {
            hit = middle;
        } else {
            free = middle;
        }
    }
    return along(free);
}

void CharacterMoveSystem::update(float delta_time) {
    std::vector<core::EntityId> changed;
    registry_.each<Character, Position, Mover>(
        [this, delta_time, &changed](core::EntityId id, Character& character,
                                     Position& position, Mover& mover) {
            const float length = std::hypot(mover.direction.x, mover.direction.y);
            const bool moving = length > 0.0f;
            if (moving) {
                const float speed = mover.running ? mover.speed * RunSpeedMultiplier : mover.speed;
                const float step = speed * delta_time / length;
                move(id, position, {mover.direction.x * step, mover.direction.y * step});
            }

            CharacterState state = CharacterState::Idle;
            if (moving) {
                state = mover.running ? CharacterState::Running : CharacterState::Moving;
            }
            if (character.state == state) {
                return;
            }
            character.state = state;
            changed.push_back(id);
        });

    // Published after the loop so subscribers are free to change the registry.
    for (const core::EntityId id : changed) {
        bus_.publish(CharacterStateChanged{id});
    }
}

} // namespace world
