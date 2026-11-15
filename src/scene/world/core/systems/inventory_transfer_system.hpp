#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// How close (in tiles, centre to centre) two entities must be to pass items between their
// inventories.
inline constexpr float TransferReach = 2.0f;

// Moves items between inventories: on each ItemTransferRequested whose two entities both
// have an Inventory and a Position within TransferReach, it runs transfer_items(). Other
// requests change nothing. The registry and bus must outlive it.
class InventoryTransferSystem : public System {
public:
    InventoryTransferSystem(core::Registry& registry, event::EventBus& bus);

    void update(float) override {}

private:
    core::Registry& registry_;
    event::ScopedSubscription subscription_;
};

// True when both entities have a Position and are within TransferReach of each other.
bool within_transfer_reach(const core::Registry& registry, core::EntityId a, core::EntityId b);

} // namespace world
