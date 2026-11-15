#include "scene/world/core/systems/build_system.hpp"

#include <vector>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/construction/build_rules.hpp"
#include "scene/world/core/events/building_events.hpp"

namespace world {

BuildSystem::BuildSystem(core::Registry& registry, event::EventBus& bus, const Terrain& terrain,
                         const EntityFactory& factory, core::EntityId payer)
    : registry_(registry),
      terrain_(terrain),
      factory_(factory),
      payer_(payer),
      subscription_(event::subscribe_scoped<BuildRequested>(bus, [this](const BuildRequested& request) {
          const Construction* construction = find_construction(request.type);
          if (construction == nullptr) {
              return;
          }
          build(registry_, factory_, *construction, request.tile, terrain_, payer_);
      })) {}

ConstructionProgressSystem::ConstructionProgressSystem(core::Registry& registry,
                                                       event::EventBus& bus)
    : registry_(registry), bus_(bus) {}

void ConstructionProgressSystem::update(float delta_time) {
    std::vector<core::EntityId> finished;
    registry_.each<ConstructionSite>([&](core::EntityId id, ConstructionSite& site) {
        site.progress += delta_time;
        if (site.progress >= site.effort) {
            finished.push_back(id);
        }
    });
    for (const core::EntityId id : finished) {
        registry_.remove<ConstructionSite>(id);
        bus_.publish(BuildingCompleted{id});
    }
}

} // namespace world
