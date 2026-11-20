#include "scene/world/core/entities/worker.hpp"

#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/unit.hpp"

namespace world {

core::EntityId spawn_worker(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id = spawn_unit(registry, std::move(label), position);
    registry.add(id, Worker{});
    registry.add(id, SolidInventory{WorkerInventorySlots});
    return id;
}

} // namespace world
