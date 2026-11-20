#include "scene/world/core/systems/route_system.hpp"

#include "scene/world/core/events/building_events.hpp"

namespace world {

RouteSystem::RouteSystem(const core::Registry& registry, event::EventBus& bus,
                         const Terrain& terrain, RouteNetwork& network)
    : registry_(registry),
      terrain_(terrain),
      network_(network),
      subscription_(event::subscribe_scoped<BuildingPlaced>(
          bus, [this](const BuildingPlaced&) { dirty_ = true; })),
      completed_(event::subscribe_scoped<BuildingCompleted>(
          bus, [this](const BuildingCompleted&) { dirty_ = true; })) {}

void RouteSystem::update(float) {
    if (!dirty_) {
        return;
    }
    dirty_ = false;
    network_.rebuild(registry_, terrain_);
}

} // namespace world
