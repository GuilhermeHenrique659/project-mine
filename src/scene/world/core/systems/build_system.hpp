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

// Advances every ConstructionSite by the time step (one effort unit per second). A site
// that reaches its effort is removed, which finishes the building, and BuildingCompleted
// is published for it.
class ConstructionProgressSystem : public System {
public:
    ConstructionProgressSystem(core::Registry& registry, event::EventBus& bus);

    void update(float delta_time) override;

private:
    core::Registry& registry_;
    event::EventBus& bus_;
};

} // namespace world
