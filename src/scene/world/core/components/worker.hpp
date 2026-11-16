#pragma once

#include <optional>

#include "core/entity.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/components/building.hpp"

namespace world {

// What a worker is doing: Idle = free to take a job (standing at or walking back to the
// headquarters), Working = assigned to a job (walking to it or doing it).
enum class WorkerState { Idle, Working };

// Effort of a construction site per worker allowed on it: a site takes effort / 10
// workers at most (a mine of effort 300 takes 30).
inline constexpr float EffortPerWorker = 10.0f;

// Workers the headquarters brings when it is placed.
inline constexpr int HeadquartersWorkers = 100;

// Marks a character as a worker driven by the WorkerSystem (never by the player). Its job
// (the site) is the Builder's site. Only the marker is saved; a loaded worker is idle.
struct Worker {
    WorkerState state = WorkerState::Idle;
    // The production building it is assigned to (0 = none; a construction job is the
    // Builder's site instead). Assigned only by the player (WorkerAssignmentRequested).
    core::EntityId workplace = 0;
};

// A building that needs workers to produce: with `needed` workers at it (within
// BuildReach) it runs at its full rate, with fewer at rate * present / needed, with none
// it is idle (NoWorkers). Workers are never sent automatically: the player assigns them.
struct Staffing {
    int needed = 0;
};

// A building that brings `workers` new idle workers, spawned on it once its construction
// is finished (BuildingCompleted). Loading a save spawns none: the workers are saved.
struct Housing {
    int workers = 0;
};

// A loaded worker's links to buildings, by tile (entity ids are not saved, and the building
// may be loaded after the worker): its workplace, its construction site and the building at
// the end of the route it walks (from the headquarters), with the walking direction. The
// WorkerSystem turns them into ids on its next update and removes this component.
struct PendingWorkerLinks {
    std::optional<TileCoord> workplace = std::nullopt;
    std::optional<TileCoord> site = std::nullopt;
    std::optional<TileCoord> route = std::nullopt;
    bool forward = true;
};

// How many workers may build `site` at the same time.
int max_workers(const ConstructionSite& site);

constexpr const char* to_string(WorkerState state) {
    return state == WorkerState::Working ? "Working" : "Idle";
}

} // namespace world
