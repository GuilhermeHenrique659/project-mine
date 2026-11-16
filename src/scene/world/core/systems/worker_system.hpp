#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Drives the workers:
// - when a headquarters is placed (BuildingPlaced) it spawns HeadquartersWorkers on it;
// - each update, an idle worker takes the first construction site with room (max_workers):
//   it becomes Working, the site becomes its Builder's site and it follows the route
//   headquarters -> site (the ConstructionWorkSystem builds once it is in reach);
// - when its site is finished (or gone) it turns Idle and walks the route back to the
//   headquarters, where it waits; on the way it can already take a new site;
// - on WorkerAssignmentRequested it sends idle workers to a finished building with
//   Staffing (never beyond what it needs) or, with a negative count, sends that many of its
//   workers back idle. Production workers are only ever assigned this way.
// Runs before RouteFollowSystem. Everything passed must outlive it.
class WorkerSystem : public System {
public:
    WorkerSystem(core::Registry& registry, event::EventBus& bus, const EntityFactory& factory);

    void update(float delta_time) override;

private:
    void spawn_workers(core::EntityId headquarters);
    // Turns the PendingWorkerLinks of loaded workers into ids, routes included.
    void resolve_loaded(core::EntityId headquarters);
    void assign(core::EntityId building, int count);
    void release(core::EntityId building, int count);
    // Sends the worker along the route headquarters -> target (rejoining it where it stands).
    void send(core::EntityId worker, core::EntityId headquarters, core::EntityId target);

    core::Registry& registry_;
    const EntityFactory& factory_;
    event::ScopedSubscription subscription_;
    event::ScopedSubscription assignment_subscription_;
};

} // namespace world
