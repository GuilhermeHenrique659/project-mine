#include "scene/world/core/entities/character.hpp"

#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"

namespace world {

core::EntityId spawn_character(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id = registry.create(std::move(label));
    registry.add(id, Character{});
    registry.add(id, Position{position});
    registry.add(id, Mover{CharacterSpeed});
    return id;
}

} // namespace world
