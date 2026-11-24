#include "scene/world/core/systems/move_order_system.hpp"

#include <utility>
#include <vector>

#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/entities/vehicle.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/pathfinding/pathfinder.hpp"
#include "scene/world/core/selection.hpp"

namespace world {

MoveOrderSystem::MoveOrderSystem(core::Registry& registry, event::EventBus& bus,
                                 const Terrain& terrain)
    : registry_(registry),
      terrain_(terrain),
      subscription_(event::subscribe_scoped<UnitsMoveRequested>(
          bus, [this](const UnitsMoveRequested& request) { order(request.units, request.goal); })),
      approach_subscription_(event::subscribe_scoped<UnitsApproachRequested>(
          bus, [this](const UnitsApproachRequested& request) {
              order(request.units, request.goal);
          })) {}

void MoveOrderSystem::order(const std::vector<core::EntityId>& units, TileCoord goal) {
    std::vector<core::EntityId> soldiers;
    for (const core::EntityId id : units) {
        if (takes_orders(registry_, id) && registry_.has<Position>(id)) {
            soldiers.push_back(id);
        }
    }
    if (soldiers.empty()) {
        return;
    }
    const BlockedFn is_blocked = [this](TileCoord tile) {
        return building_at(registry_, tile) != 0 || terrain_.tile_at(tile) == TileType::Water;
    };
    const std::vector<TileCoord> goals = formation_tiles(goal, soldiers.size(), is_blocked);
    for (std::size_t i = 0; i < soldiers.size(); ++i) {
        const core::EntityId id = soldiers[i];
        registry_.remove<PathFollower>(id);
        if (i >= goals.size()) {
            continue;
        }
        const TileCoord start = tile_containing(registry_.get<Position>(id)->point);
        auto path = find_path(start, goals[i], is_blocked);
        if (path) {
            registry_.add(id, PathFollower{std::move(*path)});
        }
    }
}

} // namespace world
