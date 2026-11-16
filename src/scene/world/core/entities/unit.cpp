#include "scene/world/core/entities/unit.hpp"

#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/entities/character.hpp"

namespace world {

core::EntityId spawn_unit(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id = spawn_character(registry, std::move(label), position);
    registry.add(id, Unit{});
    return id;
}

} // namespace world
