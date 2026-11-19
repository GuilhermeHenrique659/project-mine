#include "scene/world/core/systems/recruit_system.hpp"

#include <algorithm>
#include <optional>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/spawn.hpp"

namespace world {

RecruitSystem::RecruitSystem(core::Registry& registry, event::EventBus& bus,
                             const EntityFactory& factory, const Terrain& terrain)
    : registry_(registry),
      factory_(factory),
      terrain_(terrain),
      subscription_(event::subscribe_scoped<SoldierRecruitRequested>(
          bus, [this](const SoldierRecruitRequested& request) { recruit(request.building); })) {}

void RecruitSystem::recruit(core::EntityId building) {
    const auto* at = registry_.get<Position>(building);
    const auto* footprint = registry_.get<Building>(building);
    if (at == nullptr || footprint == nullptr || !registry_.has<Recruiter>(building) ||
        registry_.has<ConstructionSite>(building)) {
        return;
    }
    const auto* power = registry_.get<PowerConsumer>(building);
    if (power != nullptr && !power->powered) {
        return;
    }
    // Out of the barracks on a free tile; on the building itself only when none is free.
    WorldPoint point = at->point;
    if (const auto tile = spawn_tile(registry_, terrain_, *footprint)) {
        point = {tile->x + 0.5f, tile->y + 0.5f};
    }
    const auto id = factory_.spawn(registry_, SoldierArchetype, "Soldier");
    if (!id) {
        return;
    }
    registry_.get<Position>(*id)->point = point;
}

} // namespace world
