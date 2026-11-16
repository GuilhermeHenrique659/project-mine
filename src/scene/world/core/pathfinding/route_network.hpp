#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "core/entity.hpp"
#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/pathfinding/pathfinder.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Which pair of buildings a route links. Stays the same across recalculations, so a
// follower keeps its route when the network is rebuilt.
struct RouteKey {
    core::EntityId from = 0;
    core::EntityId to = 0;

    bool operator==(const RouteKey&) const = default;
};

// What a route is for: Worker routes take workers from the headquarters to their
// building; Logistic routes link a storage to a production building, for carrying items
// (listed in the Logistics tab of the player menu).
enum class RouteKind { Worker, Logistic };

// A route a planner asks for.
struct RouteRequest {
    RouteKey key;
    RouteKind kind = RouteKind::Worker;
};

// A precomputed walk between two buildings: every tile from the `from` building's tile to
// the `to` building's tile, both included. Empty when no path was found.
struct Route {
    RouteKey key;
    std::vector<TileCoord> tiles;
    RouteKind kind = RouteKind::Worker;

    bool reachable() const { return !tiles.empty(); }
};

// Decides which pairs of buildings get a route. A new combination (mine -> factory, every
// pair, ...) = a new subclass passed to the RouteNetwork; nothing else changes.
class RoutePlanner {
public:
    virtual ~RoutePlanner() = default;

    virtual std::vector<RouteRequest> plan(const core::Registry& registry) const = 0;
};

// One Worker route from the headquarters to every other building (none without a
// headquarters).
class HeadquartersRoutePlanner : public RoutePlanner {
public:
    std::vector<RouteRequest> plan(const core::Registry& registry) const override;
};

// One Logistic route from every storage to every production building (one with a
// Producer, finished or not).
class LogisticRoutePlanner : public RoutePlanner {
public:
    std::vector<RouteRequest> plan(const core::Registry& registry) const override;
};

// The routes of several planners, in their order.
class CompositeRoutePlanner : public RoutePlanner {
public:
    explicit CompositeRoutePlanner(std::vector<std::unique_ptr<RoutePlanner>> planners);

    std::vector<RouteRequest> plan(const core::Registry& registry) const override;

private:
    std::vector<std::unique_ptr<RoutePlanner>> planners_;
};

// The game's planner: the headquarters' Worker routes, then the Logistic routes.
std::unique_ptr<RoutePlanner> default_route_planner();

// The routes between buildings, computed only by rebuild(), never per frame. Walking
// avoids water and every building tile (sites included), except the route's own ends.
class RouteNetwork {
public:
    explicit RouteNetwork(std::unique_ptr<RoutePlanner> planner,
                          int max_search_nodes = MaxPathSearchNodes);

    // Recomputes every route the planner asks for and bumps the revision.
    void rebuild(const core::Registry& registry, const Terrain& terrain);

    // The route for this pair, or null when the planner did not ask for it.
    const Route* find(RouteKey key) const;
    const std::vector<Route>& routes() const { return routes_; }

    // Grows on each rebuild; followers compare it to notice that their route changed.
    std::uint64_t revision() const { return revision_; }

private:
    std::unique_ptr<RoutePlanner> planner_;
    int max_search_nodes_;
    std::vector<Route> routes_;
    std::uint64_t revision_ = 0;
};

} // namespace world
