#pragma once

#include <string>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// A finished building with Staffing and the workers assigned to it (walking there or at it).
struct WorkplaceEntry {
    core::EntityId id = 0;
    std::string name;
    BuildingType type = BuildingType::Headquarters;
    TileCoord tile{};
    int assigned = 0;
    int needed = 0;
};

// The workers as the player sees them: how many are idle and every workplace in registry
// order.
struct WorkerReport {
    int idle = 0;
    std::vector<WorkplaceEntry> workplaces;
};

WorkerReport worker_report(const core::Registry& registry);

// Whether the worker is doing its job right now: Working and within BuildReach of its
// workplace or construction site (walking there does not count). Such a worker is inside
// the building and is not drawn.
bool is_at_work(const core::Registry& registry, core::EntityId worker);

} // namespace world
