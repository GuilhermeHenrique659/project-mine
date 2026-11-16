#include "scene/world/core/worker_report.hpp"

#include <cmath>

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

WorkerReport worker_report(const core::Registry& registry) {
    WorkerReport report;
    for (const core::Entity& entity : registry.entities()) {
        const auto* worker = registry.get<Worker>(entity.id);
        if (worker != nullptr && worker->state == WorkerState::Idle) {
            ++report.idle;
        }
        const auto* staffing = registry.get<Staffing>(entity.id);
        const auto* building = registry.get<Building>(entity.id);
        if (staffing == nullptr || building == nullptr ||
            registry.has<ConstructionSite>(entity.id)) {
            continue;
        }
        report.workplaces.push_back({entity.id, entity.label, building->tile, 0, staffing->needed});
    }
    for (const core::Entity& entity : registry.entities()) {
        const auto* worker = registry.get<Worker>(entity.id);
        if (worker == nullptr || worker->workplace == 0) {
            continue;
        }
        for (WorkplaceEntry& entry : report.workplaces) {
            entry.assigned += entry.id == worker->workplace ? 1 : 0;
        }
    }
    return report;
}

bool is_at_work(const core::Registry& registry, core::EntityId id) {
    const auto* worker = registry.get<Worker>(id);
    const auto* at = registry.get<Position>(id);
    if (worker == nullptr || at == nullptr || worker->state != WorkerState::Working) {
        return false;
    }
    core::EntityId job = worker->workplace;
    if (job == 0) {
        const auto* builder = registry.get<Builder>(id);
        job = builder != nullptr ? builder->site : 0;
    }
    const auto* job_at = registry.get<Position>(job);
    if (job == 0 || job_at == nullptr) {
        return false;
    }
    return distance_to_building(registry, job, at->point) <= BuildReach;
}

} // namespace world
