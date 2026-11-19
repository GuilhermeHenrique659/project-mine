#include "scene/world/core/entities/soldier.hpp"

#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/entities/unit.hpp"

namespace world {

core::EntityId spawn_soldier(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id = spawn_unit(registry, std::move(label), position);
    registry.add(id, Soldier{});
    registry.add(id, Shooter{});
    registry.add(id, Inventory{SoldierInventorySlots});
    return id;
}

} // namespace world
