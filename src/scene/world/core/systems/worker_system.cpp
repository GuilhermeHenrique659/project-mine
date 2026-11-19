#include "scene/world/core/systems/worker_system.hpp"

#include "scene/world/core/spawn.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/core/events/worker_events.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

int max_workers(const ConstructionSite& site) {
    return static_cast<int>(site.effort / EffortPerWorker);
}

namespace {

core::EntityId find_headquarters(const core::Registry& registry) {
    for (const core::Entity& entity : registry.entities()) {
        const auto* building = registry.get<Building>(entity.id);
        if (building != nullptr && building->type == BuildingType::Headquarters &&
            !registry.has<ConstructionSite>(entity.id)) {
            return entity.id;
        }
    }
    return 0;
}

core::EntityId building_on(const core::Registry& registry, std::optional<TileCoord> tile) {
    return tile ? building_at(registry, *tile) : 0;
}

} // namespace

void WorkerSystem::resolve_loaded(core::EntityId headquarters) {
    std::vector<core::EntityId> loaded;
    registry_.each<PendingWorkerLinks>(
        [&](core::EntityId id, PendingWorkerLinks&) { loaded.push_back(id); });
    for (const core::EntityId id : loaded) {
        const PendingWorkerLinks links = *registry_.get<PendingWorkerLinks>(id);
        registry_.remove<PendingWorkerLinks>(id);
        Worker* worker = registry_.get<Worker>(id);
        Builder* builder = registry_.get<Builder>(id);
        if (worker == nullptr || builder == nullptr) {
            continue;
        }
        const core::EntityId workplace = building_on(registry_, links.workplace);
        const core::EntityId site = building_on(registry_, links.site);
        worker->workplace =
            registry_.has<Staffing>(workplace) && !registry_.has<ConstructionSite>(workplace)
                ? workplace
                : 0;
        builder->site = registry_.has<ConstructionSite>(site) ? site : 0;
        // A loaded hauler stays Working: the HaulSystem restores its job.
        if (worker->workplace == 0 && builder->site == 0 &&
            !registry_.has<PendingHaulerLinks>(id)) {
            worker->state = WorkerState::Idle;
        }
        const core::EntityId route_end = building_on(registry_, links.route);
        if (headquarters == 0 || route_end == 0) {
            continue;
        }
        send(id, headquarters, route_end);
        registry_.get<RouteFollower>(id)->forward = links.forward;
    }
}

WorkerSystem::WorkerSystem(core::Registry& registry, event::EventBus& bus,
                           const EntityFactory& factory, const Terrain& terrain)
    : registry_(registry),
      factory_(factory),
      terrain_(terrain),
      subscription_(event::subscribe_scoped<BuildingPlaced>(bus, [this](const BuildingPlaced& placed) {
          const core::EntityId id = building_at(registry_, placed.tile);
          const auto* building = registry_.get<Building>(id);
          if (building != nullptr && building->type == BuildingType::Headquarters) {
              spawn_workers(id, HeadquartersWorkers);
          }
      })),
      assignment_subscription_(event::subscribe_scoped<WorkerAssignmentRequested>(
          bus, [this](const WorkerAssignmentRequested& request) {
              if (request.count > 0) {
                  assign(request.building, request.count);
              } else {
                  release(request.building, -request.count);
              }
          })),
      completion_subscription_(event::subscribe_scoped<BuildingCompleted>(
          bus, [this](const BuildingCompleted& completed) {
              if (const auto* housing = registry_.get<Housing>(completed.entity)) {
                  spawn_workers(completed.entity, housing->workers);
              }
          })) {}

void WorkerSystem::send(core::EntityId worker, core::EntityId headquarters,
                        core::EntityId target) {
    // Revision 0: the follower joins the route at the waypoint nearest to where it stands.
    const RouteFollower follower{RouteKey{headquarters, target}};
    if (auto* existing = registry_.get<RouteFollower>(worker)) {
        *existing = follower;
        return;
    }
    registry_.add(worker, follower);
}

void WorkerSystem::assign(core::EntityId building, int count) {
    const core::EntityId headquarters = find_headquarters(registry_);
    const auto* staffing = registry_.get<Staffing>(building);
    if (headquarters == 0 || staffing == nullptr || registry_.has<ConstructionSite>(building)) {
        return;
    }
    int assigned = 0;
    registry_.each<Worker>([&](core::EntityId, Worker& worker) {
        assigned += worker.workplace == building ? 1 : 0;
    });
    int wanted = std::min(count, staffing->needed - assigned);
    std::vector<core::EntityId> hired;
    registry_.each<Worker>([&](core::EntityId id, Worker& worker) {
        if (wanted <= 0 || worker.state != WorkerState::Idle) {
            return;
        }
        worker.state = WorkerState::Working;
        worker.workplace = building;
        hired.push_back(id);
        --wanted;
    });
    for (const core::EntityId id : hired) {
        send(id, headquarters, building);
    }
}

void WorkerSystem::release(core::EntityId building, int count) {
    registry_.each<Worker>([&](core::EntityId id, Worker& worker) {
        if (count <= 0 || worker.workplace != building) {
            return;
        }
        worker.state = WorkerState::Idle;
        worker.workplace = 0;
        if (auto* follower = registry_.get<RouteFollower>(id)) {
            follower->forward = false;
        }
        --count;
    });
}

void WorkerSystem::spawn_workers(core::EntityId building, int count) {
    const auto* at = registry_.get<Position>(building);
    if (at == nullptr) {
        return;
    }
    const WorldPoint centre = at->point;
    const auto* footprint = registry_.get<Building>(building);
    for (int i = 0; i < count; ++i) {
        // Out of the building on a free tile; on the building itself only when none is free.
        WorldPoint point = centre;
        if (footprint != nullptr) {
            if (const auto tile = spawn_tile(registry_, terrain_, *footprint)) {
                point = {tile->x + 0.5f, tile->y + 0.5f};
            }
        }
        const auto id = factory_.spawn(registry_, WorkerArchetype, "Worker");
        if (!id) {
            return;
        }
        registry_.get<Position>(*id)->point = point;
    }
}

void WorkerSystem::update(float) {
    const core::EntityId headquarters = find_headquarters(registry_);
    resolve_loaded(headquarters);

    // Finished (or vanished) jobs: back to idle, walking the route back home.
    std::unordered_map<core::EntityId, int> assigned;
    registry_.each<Worker, Builder>([&](core::EntityId id, Worker& worker, Builder& builder) {
        // Haulers belong to the HaulSystem.
        if (worker.state != WorkerState::Working || registry_.has<Hauler>(id)) {
            return;
        }
        if (worker.workplace != 0 && registry_.has<Building>(worker.workplace)) {
            return;
        }
        if (worker.workplace == 0 && registry_.has<ConstructionSite>(builder.site)) {
            ++assigned[builder.site];
            return;
        }
        worker.state = WorkerState::Idle;
        worker.workplace = 0;
        builder.site = 0;
        if (auto* follower = registry_.get<RouteFollower>(id)) {
            follower->forward = false;
        }
    });
    if (headquarters == 0) {
        return;
    }

    // Sites with room left, in registry order.
    std::vector<core::EntityId> open_sites;
    for (const core::Entity& entity : registry_.entities()) {
        const auto* site = registry_.get<ConstructionSite>(entity.id);
        if (site != nullptr && assigned[entity.id] < max_workers(*site)) {
            open_sites.push_back(entity.id);
        }
    }
    if (open_sites.empty()) {
        return;
    }

    std::vector<core::EntityId> hired;
    std::size_t next = 0;
    registry_.each<Worker, Builder>([&](core::EntityId id, Worker& worker, Builder& builder) {
        if (worker.state != WorkerState::Idle || next >= open_sites.size()) {
            return;
        }
        const core::EntityId site = open_sites[next];
        worker.state = WorkerState::Working;
        builder.site = site;
        hired.push_back(id);
        if (++assigned[site] >= max_workers(*registry_.get<ConstructionSite>(site))) {
            ++next;
        }
    });
    for (const core::EntityId id : hired) {
        send(id, headquarters, registry_.get<Builder>(id)->site);
    }
}

} // namespace world
