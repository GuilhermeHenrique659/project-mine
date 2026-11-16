#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Keeps the RouteNetwork up to date: rebuilds it on the first update (the buildings of a
// loaded save) and on the update after any BuildingPlaced, once however many were placed.
// It never recomputes otherwise. Everything passed must outlive it.
class RouteSystem : public System {
public:
    RouteSystem(const core::Registry& registry, event::EventBus& bus, const Terrain& terrain,
                RouteNetwork& network);

    void update(float delta_time) override;

private:
    const core::Registry& registry_;
    const Terrain& terrain_;
    RouteNetwork& network_;
    bool dirty_ = true;
    event::ScopedSubscription subscription_;
};

} // namespace world
