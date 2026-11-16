#include "scene/world/core/worker_type.hpp"

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/components/worker.hpp"

namespace world {

namespace {

// The worker type of a production building's workers. A new production building = a new
// case here (a steelworks' workers are steelworkers).
WorkerType type_at(BuildingType building) {
    switch (building) {
        case BuildingType::Mine: return WorkerType::Miner;
        case BuildingType::Steelworks: return WorkerType::Steelworker;
        case BuildingType::Headquarters:
        case BuildingType::Storage: break;
    }
    return WorkerType::None;
}

} // namespace

WorkerType worker_type(const core::Registry& registry, core::EntityId worker) {
    const Worker* state = registry.get<Worker>(worker);
    if (state == nullptr || state->state != WorkerState::Working) {
        return WorkerType::None;
    }
    if (registry.has<Hauler>(worker)) {
        return WorkerType::Hauler;
    }
    if (const Building* workplace = registry.get<Building>(state->workplace)) {
        return type_at(workplace->type);
    }
    const Builder* builder = registry.get<Builder>(worker);
    if (builder != nullptr && registry.has<ConstructionSite>(builder->site)) {
        return WorkerType::Builder;
    }
    return WorkerType::None;
}

} // namespace world
