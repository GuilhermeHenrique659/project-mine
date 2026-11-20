#include "scene/world/core/systems/resupply_system.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/pathfinding/pathfinder.hpp"

namespace world {

namespace {

// The free tiles touching a building's footprint (its first ring), row by row.
std::vector<TileCoord> tiles_around(const Building& building, const BlockedFn& is_blocked) {
    std::vector<TileCoord> tiles;
    const int left = building.tile.x - 1;
    const int top = building.tile.y - 1;
    const int right = building.tile.x + building.size.width;
    const int bottom = building.tile.y + building.size.height;
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            const bool border = x == left || x == right || y == top || y == bottom;
            if (border && !is_blocked({x, y})) {
                tiles.push_back({x, y});
            }
        }
    }
    return tiles;
}

int distance_squared(TileCoord a, TileCoord b) {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

// Whether the soldier lacks a weapon or ammo.
bool needs_supplies(const SolidInventory& soldier) {
    return soldier.count(items::Weapon) == 0 || soldier.count(items::Ammo) == 0;
}

// Whether `supply` holds something the soldier lacks.
bool has_what_is_missing(const SolidInventory& supply, const SolidInventory& soldier) {
    return (soldier.count(items::Weapon) == 0 && supply.count(items::Weapon) > 0) ||
           (soldier.count(items::Ammo) < items::Ammo.max_stack && supply.count(items::Ammo) > 0);
}

// One weapon when the soldier holds none, then ammo up to one full stack.
void take_supplies(SolidInventory& supply, SolidInventory& soldier) {
    if (soldier.count(items::Weapon) == 0) {
        transfer_items(supply, soldier, items::Weapon, 1);
    }
    const int missing = items::Ammo.max_stack - soldier.count(items::Ammo);
    if (missing > 0) {
        transfer_items(supply, soldier, items::Ammo, missing);
    }
}

} // namespace

ResupplySystem::ResupplySystem(core::Registry& registry, event::EventBus& bus,
                               const Terrain& terrain)
    : registry_(registry),
      terrain_(terrain),
      subscription_(event::subscribe_scoped<UnitsResupplyRequested>(
          bus, [this](const UnitsResupplyRequested& request) { order(request); })),
      move_subscription_(event::subscribe_scoped<UnitsMoveRequested>(
          bus, [this](const UnitsMoveRequested& request) {
              for (const core::EntityId id : request.units) {
                  registry_.remove<ResupplyOrder>(id);
              }
          })) {}

bool ResupplySystem::is_blocked(TileCoord tile) const {
    return building_at(registry_, tile) != 0 || terrain_.tile_at(tile) == TileType::Water;
}

void ResupplySystem::order(const UnitsResupplyRequested& request) {
    if (!registry_.has<Building>(request.supply) || !registry_.has<Supply>(request.supply)) {
        return;
    }
    for (const core::EntityId id : request.units) {
        if (registry_.has<Position>(id) && registry_.has<Soldier>(id)) {
            send(id, request.supply);
        }
    }
}

void ResupplySystem::send(core::EntityId soldier, core::EntityId supply) {
    const WorldPoint point = registry_.get<Position>(soldier)->point;
    registry_.remove<PathFollower>(soldier);
    registry_.add(soldier, ResupplyOrder{supply});
    if (distance_to_building(registry_, supply, point) <= SupplyReach) {
        return;
    }
    const BlockedFn blocked = [this](TileCoord tile) { return is_blocked(tile); };
    const TileCoord start = tile_containing(point);
    // The nearest free tile beside the building; update() drops the order without a path.
    std::vector<TileCoord> goals = tiles_around(*registry_.get<Building>(supply), blocked);
    std::sort(goals.begin(), goals.end(), [start](TileCoord a, TileCoord b) {
        return distance_squared(start, a) < distance_squared(start, b);
    });
    for (const TileCoord goal : goals) {
        auto path = find_path(start, goal, blocked);
        if (path) {
            registry_.add(soldier, PathFollower{std::move(*path)});
            return;
        }
    }
}

core::EntityId ResupplySystem::nearest_supply(WorldPoint point, const SolidInventory& carried) const {
    core::EntityId nearest = 0;
    float best = 0.0f;
    registry_.each<Supply, SolidInventory>([&](core::EntityId id, const Supply&, const SolidInventory& stock) {
        if (!registry_.has<Building>(id) || registry_.has<ConstructionSite>(id) ||
            !has_what_is_missing(stock, carried)) {
            return;
        }
        const float distance = distance_to_building(registry_, id, point);
        if (nearest == 0 || distance < best) {
            nearest = id;
            best = distance;
        }
    });
    return nearest;
}

void ResupplySystem::seek_supplies() {
    std::vector<std::pair<core::EntityId, core::EntityId>> sends;
    registry_.each<Soldier, Position, SolidInventory>(
        [&](core::EntityId id, const Soldier&, const Position& position, const SolidInventory& carried) {
            if (registry_.has<ResupplyOrder>(id) || registry_.has<PathFollower>(id) ||
                !needs_supplies(carried)) {
                return;
            }
            if (const core::EntityId supply = nearest_supply(position.point, carried)) {
                sends.emplace_back(id, supply);
            }
        });
    for (const auto& [soldier, supply] : sends) {
        send(soldier, supply);
    }
}

void ResupplySystem::update(float delta_time) {
    std::vector<core::EntityId> done;
    registry_.each<ResupplyOrder, Position, SolidInventory>(
        [&](core::EntityId id, ResupplyOrder& order, Position& position, SolidInventory& carried) {
            if (registry_.has<PathFollower>(id)) {
                return;
            }
            done.push_back(id);
            auto* supply = registry_.get<SolidInventory>(order.supply);
            if (supply == nullptr || !registry_.has<Supply>(order.supply) ||
                registry_.has<ConstructionSite>(order.supply) ||
                distance_to_building(registry_, order.supply, position.point) > SupplyReach) {
                return;
            }
            take_supplies(*supply, carried);
        });
    for (const core::EntityId id : done) {
        registry_.remove<ResupplyOrder>(id);
    }
    since_search_ += delta_time;
    if (since_search_ < AutoResupplyInterval) {
        return;
    }
    since_search_ -= AutoResupplyInterval;
    seek_supplies();
}

} // namespace world
