#include "scene/world/core/systems/build_system.hpp"

#include <cmath>
#include <vector>

#include "logging/logger.hpp"
#include "text/parse.hpp"

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/construction/build_rules.hpp"
#include "scene/world/core/events/building_events.hpp"

namespace world {

BuildSystem::BuildSystem(core::Registry& registry, event::EventBus& bus, const Terrain& terrain,
                         const EntityFactory& factory, core::EntityId payer)
    : registry_(registry),
      bus_(bus),
      terrain_(terrain),
      factory_(factory),
      payer_(payer),
      subscription_(event::subscribe_scoped<BuildRequested>(bus, [this](const BuildRequested& request) {
          const Construction* construction = find_construction(request.type);
          if (construction == nullptr) {
              return;
          }
          if (build(registry_, factory_, *construction, request.tile, terrain_, payer_) !=
              BuildResult::Ok) {
              return;
          }
          logging::info("Placed " + construction->name + " at (" + text::format(request.tile.x) +
                        ", " + text::format(request.tile.y) + ")");
          bus_.publish(BuildingPlaced{request.tile});
      })) {}

ConstructionWorkSystem::ConstructionWorkSystem(core::Registry& registry, event::EventBus& bus)
    : registry_(registry), bus_(bus) {}

void ConstructionWorkSystem::update(float delta_time) {
    std::vector<core::EntityId> finished;
    registry_.each<Builder, Position>([&](core::EntityId, Builder& builder, Position& at) {
        ConstructionSite* site = registry_.get<ConstructionSite>(builder.site);
        const Position* site_at = registry_.get<Position>(builder.site);
        if (site == nullptr || site_at == nullptr) {
            return;
        }
        const float distance =
            std::hypot(site_at->point.x - at.point.x, site_at->point.y - at.point.y);
        if (distance > BuildReach || site->progress >= site->effort) {
            return;
        }
        site->progress += builder.rate * delta_time;
        if (site->progress >= site->effort) {
            finished.push_back(builder.site);
        }
    });
    for (const core::EntityId id : finished) {
        registry_.remove<ConstructionSite>(id);
        registry_.each<Builder>([id](core::EntityId, Builder& builder) {
            if (builder.site == id) {
                builder.site = 0;
            }
        });
        bus_.publish(BuildingCompleted{id});
    }
}

} // namespace world
