#include "scene/world/core/systems/inventory_transfer_system.hpp"

#include <cmath>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/events/item_transfer_requested.hpp"

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
    return std::hypot(at_a->point.x - at_b->point.x, at_a->point.y - at_b->point.y) <=
           TransferReach;
}

} // namespace world
