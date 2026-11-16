#include "scene/world/core/pathfinding/route_network.hpp"

#include <algorithm>
#include <utility>

#include "logging/logger.hpp"
#include "scene/world/core/components/building.hpp"
#include "text/parse.hpp"

namespace world {

std::vector<RouteKey> HeadquartersRoutePlanner::plan(const core::Registry& registry) const {
    const auto& entities = registry.entities();
    const auto hq = std::find_if(entities.begin(), entities.end(), [&](const core::Entity& entity) {
        const Building* building = registry.get<Building>(entity.id);
        return building != nullptr && building->type == BuildingType::Headquarters;
    });
    if (hq == entities.end()) {
        return {};
    }
    std::vector<RouteKey> keys;
    for (const core::Entity& entity : entities) {
        if (entity.id != hq->id && registry.has<Building>(entity.id)) {
            keys.push_back({hq->id, entity.id});
        }
    }
    return keys;
}

RouteNetwork::RouteNetwork(std::unique_ptr<RoutePlanner> planner, int max_search_nodes)
    : planner_(std::move(planner)), max_search_nodes_(max_search_nodes) {}

void RouteNetwork::rebuild(const core::Registry& registry, const Terrain& terrain) {
    std::vector<TileCoord> building_tiles;
    for (const core::Entity& entity : registry.entities()) {
        if (const Building* building = registry.get<Building>(entity.id)) {
            building_tiles.push_back(building->tile);
        }
    }
    const BlockedFn is_blocked = [&](TileCoord tile) {
        return terrain.tile_at(tile) == TileType::Water ||
               std::find(building_tiles.begin(), building_tiles.end(), tile) != building_tiles.end();
    };

    routes_.clear();
    for (const RouteKey key : planner_->plan(registry)) {
        const Building* from = registry.get<Building>(key.from);
        const Building* to = registry.get<Building>(key.to);
        if (from == nullptr || to == nullptr) {
            continue;
        }
        auto path = find_path(from->tile, to->tile, is_blocked, max_search_nodes_);
        routes_.push_back({key, path ? std::move(*path) : std::vector<TileCoord>{}});
    }
    ++revision_;

    const auto unreachable = std::count_if(routes_.begin(), routes_.end(),
                                           [](const Route& route) { return !route.reachable(); });
    logging::info("Routes recalculated (revision " + text::format(revision_) + "): " +
                  text::format(routes_.size()) + " routes, " + text::format(unreachable) +
                  " unreachable");
}

const Route* RouteNetwork::find(RouteKey key) const {
    const auto found =
        std::find_if(routes_.begin(), routes_.end(), [key](const Route& route) { return route.key == key; });
    return found == routes_.end() ? nullptr : &*found;
}

} // namespace world
