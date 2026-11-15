#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Places buildings: on each BuildRequested it runs build() for the catalog entry of the
// type, paid by `payer`. Refused requests change nothing. Everything passed must outlive it.
class BuildSystem : public System {
public:
    BuildSystem(core::Registry& registry, event::EventBus& bus, const Terrain& terrain,
                const EntityFactory& factory, core::EntityId payer);

    void update(float) override {}

private:
    core::Registry& registry_;
    const Terrain& terrain_;
    const EntityFactory& factory_;
    core::EntityId payer_;
    event::ScopedSubscription subscription_;
};

// Builders work on their site: each one with a site in reach (BuildReach) adds rate * dt to
// its progress. A site that reaches its effort is removed, which finishes the building;
// BuildingCompleted is published for it and its builders stop working on it.
class ConstructionWorkSystem : public System {
public:
    ConstructionWorkSystem(core::Registry& registry, event::EventBus& bus);

    void update(float delta_time) override;

private:
    core::Registry& registry_;
    event::EventBus& bus_;
};

} // namespace world
