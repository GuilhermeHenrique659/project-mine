#include "scene/world/core/entities/entity_factory.hpp"

#include "scene/world/core/components/persistent.hpp"
#include "scene/world/core/entities/character.hpp"

namespace world {

void EntityFactory::register_archetype(std::string archetype, Spawner spawner) {
    spawners_.insert_or_assign(std::move(archetype), std::move(spawner));
}

std::optional<core::EntityId> EntityFactory::spawn(core::Registry& registry,
                                                   const std::string& archetype,
                                                   const std::string& label) const {
    const auto found = spawners_.find(archetype);
    if (found == spawners_.end()) {
        return std::nullopt;
    }
    const core::EntityId id = found->second(registry, label);
    registry.add(id, Persistent{archetype});
    return id;
}

EntityFactory make_entity_factory() {
    EntityFactory factory;
    factory.register_archetype("character", [](core::Registry& registry, const std::string& label) {
        return spawn_character(registry, label, WorldPoint{0.0f, 0.0f});
    });
    return factory;
}

} // namespace world
