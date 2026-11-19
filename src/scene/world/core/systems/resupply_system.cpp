#include "scene/world/core/systems/resupply_system.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
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

// One weapon when the soldier holds none, then ammo up to one full stack.
void take_supplies(Inventory& supply, Inventory& soldier) {
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

void ResupplySystem::order(const UnitsResupplyRequested& request) {
    const auto* building = registry_.get<Building>(request.supply);
    if (building == nullptr || !registry_.has<Supply>(request.supply)) {
        return;
    }
    const BlockedFn is_blocked = [this](TileCoord tile) {
        return building_at(registry_, tile) != 0 || terrain_.tile_at(tile) == TileType::Water;
    };
    const std::vector<TileCoord> around = tiles_around(*building, is_blocked);
    for (const core::EntityId id : request.units) {
        const auto* position = registry_.get<Position>(id);
        if (position == nullptr || !registry_.has<Soldier>(id)) {
            continue;
        }
        registry_.remove<PathFollower>(id);
        registry_.add(id, ResupplyOrder{request.supply});
        if (distance_to_building(registry_, request.supply, position->point) <= SupplyReach) {
            continue;
        }
        const TileCoord start = tile_containing(position->point);
        // The nearest free tile beside the building; update() drops the order without a path.
        std::vector<TileCoord> goals = around;
        std::sort(goals.begin(), goals.end(), [start](TileCoord a, TileCoord b) {
            return distance_squared(start, a) < distance_squared(start, b);
        });
        for (const TileCoord goal : goals) {
            auto path = find_path(start, goal, is_blocked);
            if (path) {
                registry_.add(id, PathFollower{std::move(*path)});
                break;
            }
        }
    }
}

void ResupplySystem::update(float) {
    std::vector<core::EntityId> done;
    registry_.each<ResupplyOrder, Position, Inventory>(
        [&](core::EntityId id, ResupplyOrder& order, Position& position, Inventory& carried) {
            if (registry_.has<PathFollower>(id)) {
                return;
            }
            done.push_back(id);
            auto* supply = registry_.get<Inventory>(order.supply);
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
}

} // namespace world
