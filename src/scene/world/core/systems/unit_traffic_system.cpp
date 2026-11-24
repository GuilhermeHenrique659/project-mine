#include "scene/world/core/systems/unit_traffic_system.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/components/vehicle.hpp"

namespace world {

namespace {

// Below this share of its normal step a walking unit counts as not getting anywhere.
constexpr float StuckProgress = 0.1f;

float distance(WorldPoint a, WorldPoint b) { return std::hypot(b.x - a.x, b.y - a.y); }

std::uint64_t key_of(TileCoord tile) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(tile.x)) << 32) |
           static_cast<std::uint32_t>(tile.y);
}

} // namespace

void UnitTrafficSystem::track(UnitTraffic& traffic, WorldPoint at, const Mover& mover,
                              bool moving, float delta_time) {
    if (traffic.ghost && distance(at, traffic.ghost_from) >= 1.0f) {
        traffic.ghost = false;
    }
    if (!moving) {
        traffic.stuck = 0.0f;
    } else if (traffic.tracked) {
        // How far it got since the last update, against how far it should have.
        const float expected = mover.speed * delta_time;
        const bool progressed = distance(traffic.last, at) >= StuckProgress * expected;
        traffic.stuck = progressed ? 0.0f : traffic.stuck + delta_time;
    }
    if (!traffic.ghost && traffic.stuck >= StuckTime) {
        traffic.ghost = true;
        traffic.ghost_from = at;
        traffic.stuck = 0.0f;
    }
    traffic.last = at;
    traffic.tracked = true;
}

void UnitTrafficSystem::update(float delta_time) {
    // Every moving unit that is not a ghost, indexed by the tile it stands on.
    std::vector<Walker> walkers;
    // Every unit that is not a ghost, walking or standing (heading unused).
    std::vector<Walker> bodies;
    registry_.each<Unit, Position, Mover>(
        [&](core::EntityId id, Unit&, Position& position, Mover& mover) {
            // Passengers sit inside their vehicle: no body in traffic.
            if (registry_.has<Passenger>(id)) {
                return;
            }
            const float length = std::hypot(mover.direction.x, mover.direction.y);
            if (!registry_.has<UnitTraffic>(id)) {
                registry_.add(id, UnitTraffic{});
            }
            UnitTraffic& traffic = *registry_.get<UnitTraffic>(id);
            track(traffic, position.point, mover, length > 0.0f, delta_time);
            traffic.spread = {};
            if (!traffic.ghost) {
                bodies.push_back({id, position.point, {}});
            }
            if (length <= 0.0f || traffic.ghost) {
                return;
            }
            walkers.push_back(
                {id, position.point, {mover.direction.x / length, mover.direction.y / length}});
        });
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> by_tile;
    for (std::size_t i = 0; i < walkers.size(); ++i) {
        by_tile[key_of(tile_containing(walkers[i].at))].push_back(i);
    }

    for (const Walker& walker : walkers) {
        // Right of the heading (y grows downwards on screen).
        const WorldPoint right{-walker.heading.y, walker.heading.x};
        float sideways = 0.0f;
        const TileCoord tile = tile_containing(walker.at);
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const auto found = by_tile.find(key_of({tile.x + dx, tile.y + dy}));
                if (found == by_tile.end()) {
                    continue;
                }
                for (const std::size_t j : found->second) {
                    const Walker& other = walkers[j];
                    if (other.id == walker.id) {
                        continue;
                    }
                    const WorldPoint away{walker.at.x - other.at.x, walker.at.y - other.at.y};
                    const float distance = std::hypot(away.x, away.y);
                    if (distance >= SeparationRadius) {
                        continue;
                    }
                    const float strength = 1.0f - distance / SeparationRadius;
                    const float side = distance > 0.0f
                                           ? (away.x * right.x + away.y * right.y) / distance
                                           : 0.0f;
                    // Straight ahead or behind: step to the right rather than slow down.
                    sideways += strength * (std::abs(side) > 1e-4f ? side : 1.0f);
                }
            }
        }
        if (sideways == 0.0f) {
            continue;
        }
        sideways = std::clamp(sideways, -MaxSidewaysPush, MaxSidewaysPush);
        registry_.get<Mover>(walker.id)->direction = {walker.heading.x + right.x * sideways,
                                                       walker.heading.y + right.y * sideways};
    }
    spread(bodies);
}

void UnitTrafficSystem::spread(const std::vector<Walker>& bodies) {
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> by_tile;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        by_tile[key_of(tile_containing(bodies[i].at))].push_back(i);
    }
    for (const Walker& body : bodies) {
        const Mover* mover = registry_.get<Mover>(body.id);
        if (std::hypot(mover->direction.x, mover->direction.y) > 0.0f) {
            continue; // only standing units are spread
        }
        WorldPoint push{};
        const TileCoord tile = tile_containing(body.at);
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const auto found = by_tile.find(key_of({tile.x + dx, tile.y + dy}));
                if (found == by_tile.end()) {
                    continue;
                }
                for (const std::size_t j : found->second) {
                    const Walker& other = bodies[j];
                    if (other.id == body.id) {
                        continue;
                    }
                    WorldPoint away{body.at.x - other.at.x, body.at.y - other.at.y};
                    float length = std::hypot(away.x, away.y);
                    if (length >= SeparationRadius) {
                        continue;
                    }
                    if (length <= 0.0f) {
                        // Same spot: split them by id so they go opposite ways.
                        away = {body.id < other.id ? -1.0f : 1.0f, 0.0f};
                        length = 1.0f;
                    }
                    const float strength = 1.0f - std::hypot(body.at.x - other.at.x,
                                                             body.at.y - other.at.y) /
                                                      SeparationRadius;
                    push.x += away.x / length * strength;
                    push.y += away.y / length * strength;
                }
            }
        }
        const float length = std::hypot(push.x, push.y);
        if (length <= 0.0f) {
            continue;
        }
        const float speed = SpreadSpeed * std::min(length, 1.0f) / length;
        registry_.get<UnitTraffic>(body.id)->spread = {push.x * speed, push.y * speed};
    }
}

} // namespace world
