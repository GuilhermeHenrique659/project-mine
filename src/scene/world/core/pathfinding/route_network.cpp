#include "scene/world/core/pathfinding/route_network.hpp"

#include <algorithm>
#include <utility>

#include "logging/logger.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/occupancy.hpp"
#include "text/parse.hpp"

namespace world {

std::vector<RouteRequest> HeadquartersRoutePlanner::plan(const core::Registry& registry) const {
    const auto& entities = registry.entities();
    const auto hq = std::find_if(entities.begin(), entities.end(), [&](const core::Entity& entity) {
        const Building* building = registry.get<Building>(entity.id);
        return building != nullptr && building->type == BuildingType::Headquarters;
    });
    if (hq == entities.end()) {
        return {};
    }
    std::vector<RouteRequest> keys;
    for (const core::Entity& entity : entities) {
        if (entity.id != hq->id && registry.has<Building>(entity.id)) {
            keys.push_back({{hq->id, entity.id}, RouteKind::Worker});
        }
    }
    return keys;
}

std::vector<RouteRequest> LogisticRoutePlanner::plan(const core::Registry& registry) const {
    std::vector<RouteRequest> requests;
    for (const core::Entity& storage : registry.entities()) {
        const Building* building = registry.get<Building>(storage.id);
        if (building == nullptr || building->type != BuildingType::Storage) {
            continue;
        }
        for (const core::Entity& producer : registry.entities()) {
            if (registry.has<Building>(producer.id) && registry.has<Producer>(producer.id)) {
                requests.push_back({{storage.id, producer.id}, RouteKind::Logistic});
            }
        }
    }
    return requests;
}

CompositeRoutePlanner::CompositeRoutePlanner(std::vector<std::unique_ptr<RoutePlanner>> planners)
    : planners_(std::move(planners)) {}

std::vector<RouteRequest> CompositeRoutePlanner::plan(const core::Registry& registry) const {
    std::vector<RouteRequest> requests;
    for (const auto& planner : planners_) {
        auto planned = planner->plan(registry);
        requests.insert(requests.end(), planned.begin(), planned.end());
    }
    return requests;
}

std::unique_ptr<RoutePlanner> default_route_planner() {
    std::vector<std::unique_ptr<RoutePlanner>> planners;
    planners.push_back(std::make_unique<HeadquartersRoutePlanner>());
    planners.push_back(std::make_unique<LogisticRoutePlanner>());
    return std::make_unique<CompositeRoutePlanner>(std::move(planners));
}

RouteNetwork::RouteNetwork(std::unique_ptr<RoutePlanner> planner, int max_search_nodes)
    : planner_(std::move(planner)), max_search_nodes_(max_search_nodes) {}

void RouteNetwork::rebuild(const core::Registry& registry, const Terrain& terrain) {
    routes_.clear();
    for (const RouteRequest& request : planner_->plan(registry)) {
        const RouteKey key = request.key;
        const Building* from = registry.get<Building>(key.from);
        const Building* to = registry.get<Building>(key.to);
        if (from == nullptr || to == nullptr) {
            continue;
        }
        // The route's own buildings can be walked through (it starts and ends inside them).
        const BlockedFn is_blocked = [&](TileCoord tile) {
            const core::EntityId occupant = building_at(registry, tile);
            if (occupant != 0) {
                return occupant != key.from && occupant != key.to;
            }
            return terrain.tile_at(tile) == TileType::Water;
        };
        auto path = find_path(from->tile, to->tile, is_blocked, max_search_nodes_);
        routes_.push_back({key, path ? std::move(*path) : std::vector<TileCoord>{}, request.kind});
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
