#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Recruits soldiers: on SoldierRecruitRequested at a finished (no ConstructionSite) and
// powered Recruiter, it spawns one soldier on the building. Everything passed must
// outlive it.
class RecruitSystem : public System {
public:
    RecruitSystem(core::Registry& registry, event::EventBus& bus, const EntityFactory& factory);

    void update(float) override {}

private:
    void recruit(core::EntityId building);

    core::Registry& registry_;
    const EntityFactory& factory_;
    event::ScopedSubscription subscription_;
};

} // namespace world
