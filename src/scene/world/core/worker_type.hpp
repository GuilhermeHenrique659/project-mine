#pragma once

#include "core/registry.hpp"

namespace world {

// The kind of work a worker does, from its current job: Builder on a construction site,
// Miner at a mine, Hauler on a logistic route, None while idle. Derived every time it is
// asked (never stored), so it always follows the job and needs nothing in saves.
enum class WorkerType { None, Builder, Miner, Hauler, Steelworker, Technician, Toolmaker, Munitioner, Gunsmith };

// The worker's type; None for an entity that is not a working worker.
WorkerType worker_type(const core::Registry& registry, core::EntityId worker);

constexpr const char* to_string(WorkerType type) {
    switch (type) {
        case WorkerType::Builder: return "Builder";
        case WorkerType::Miner: return "Miner";
        case WorkerType::Hauler: return "Hauler";
        case WorkerType::Steelworker: return "Steelworker";
        case WorkerType::Technician: return "Technician";
        case WorkerType::Toolmaker: return "Toolmaker";
        case WorkerType::Munitioner: return "Munitioner";
        case WorkerType::Gunsmith: return "Gunsmith";
        case WorkerType::None: break;
    }
    return "Idle";
}

} // namespace world
