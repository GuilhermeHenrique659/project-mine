#include "world/entities/character.hpp"

#include "world/components/character.hpp"
#include "world/components/mover.hpp"
#include "world/components/position.hpp"

namespace world {

core::EntityId spawn_character(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id = registry.create(std::move(label));
    registry.add(id, Character{});
    registry.add(id, Position{position});
    registry.add(id, Mover{CharacterSpeed});
    return id;
}

} // namespace world
