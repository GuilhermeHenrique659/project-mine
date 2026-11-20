#pragma once

#include <vector>

#include "core/entity.hpp"
#include "core/registry.hpp"

namespace world {

// Pipes that touch each other (4 neighbours, finished only) and the finished buildings with
// a LiquidInventory whose footprint touches one of them.
struct PipeGroup {
    std::vector<core::EntityId> pipes;
    std::vector<core::EntityId> members;
};

// The pipe groups of the world. Rebuilt from the registry when buildings change; nothing
// in it is saved (a loaded world rebuilds it).
class PipeNetwork {
public:
    void rebuild(const core::Registry& registry);

    const std::vector<PipeGroup>& groups() const { return groups_; }

private:
    std::vector<PipeGroup> groups_;
};

} // namespace world
