#include "scene/world/core/entities/character.hpp"

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/facing.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"

namespace world {

core::EntityId spawn_character(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id = registry.create(std::move(label));
    registry.add(id, Character{});
    registry.add(id, Facing{});
    registry.add(id, Position{position});
    registry.add(id, Mover{CharacterSpeed});
    registry.add(id, Inventory{CharacterInventorySlots});
    registry.add(id, Builder{});
    registry.add(id, Health{});
    return id;
}

} // namespace world
