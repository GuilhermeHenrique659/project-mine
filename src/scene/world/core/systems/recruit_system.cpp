#include "scene/world/core/systems/recruit_system.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/events/military_events.hpp"

namespace world {

RecruitSystem::RecruitSystem(core::Registry& registry, event::EventBus& bus,
                             const EntityFactory& factory)
    : registry_(registry),
      factory_(factory),
      subscription_(event::subscribe_scoped<SoldierRecruitRequested>(
          bus, [this](const SoldierRecruitRequested& request) { recruit(request.building); })) {}

void RecruitSystem::recruit(core::EntityId building) {
    const auto* at = registry_.get<Position>(building);
    if (at == nullptr || !registry_.has<Recruiter>(building) ||
        registry_.has<ConstructionSite>(building)) {
        return;
    }
    const auto* power = registry_.get<PowerConsumer>(building);
    if (power != nullptr && !power->powered) {
        return;
    }
    const WorldPoint point = at->point;
    const auto id = factory_.spawn(registry_, SoldierArchetype, "Soldier");
    if (!id) {
        return;
    }
    registry_.get<Position>(*id)->point = point;
}

} // namespace world
