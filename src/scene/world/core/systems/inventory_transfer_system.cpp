#include "scene/world/core/systems/inventory_transfer_system.hpp"

#include <algorithm>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/events/item_transfer_requested.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

InventoryTransferSystem::InventoryTransferSystem(core::Registry& registry, event::EventBus& bus)
    : registry_(registry),
      subscription_(event::subscribe_scoped<ItemTransferRequested>(
          bus, [this](const ItemTransferRequested& request) {
              Inventory* from = registry_.get<Inventory>(request.from);
              Inventory* to = registry_.get<Inventory>(request.to);
              if (from == nullptr || to == nullptr || request.from == request.to ||
                  !within_transfer_reach(registry_, request.from, request.to)) {
                  return;
              }
              transfer_items(*from, *to, request.item, request.count);
          })) {}

bool within_transfer_reach(const core::Registry& registry, core::EntityId a, core::EntityId b) {
    const Position* at_a = registry.get<Position>(a);
    const Position* at_b = registry.get<Position>(b);
    if (at_a == nullptr || at_b == nullptr) {
        return false;
    }
    // Either side may be a building: measure to the nearer footprint edge.
    return std::min(distance_to_building(registry, b, at_a->point),
                    distance_to_building(registry, a, at_b->point)) <= TransferReach;
}

} // namespace world
