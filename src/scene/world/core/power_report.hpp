#pragma once

#include <string>
#include <vector>

#include "core/registry.hpp"

namespace world {

// One generator or consumer of the power network: its entity label, its power (generated
// or demanded) and whether it is active (a generator running, a consumer powered).
struct PowerEntry {
    core::EntityId id = 0;
    std::string name;
    float kw = 0.0f;
    bool active = true;
};

// The state of the power network as the PowerSystem left it in its last update. Only
// finished buildings (no ConstructionSite) are listed, in registry order.
struct PowerReport {
    std::vector<PowerEntry> generators;
    std::vector<PowerEntry> consumers;
    float produced_kw = 0.0f; // sum of the running generators
    float demand_kw = 0.0f;   // sum of every consumer

    // Demand above production: every consumer is unpowered.
    bool overloaded() const { return demand_kw > produced_kw; }
};

PowerReport power_report(const core::Registry& registry);

} // namespace world
