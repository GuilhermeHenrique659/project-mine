#include "scene/world/core/entities/worker.hpp"

#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/character.hpp"

namespace world {

core::EntityId spawn_worker(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id = spawn_character(registry, std::move(label), position);
    registry.add(id, Worker{});
    return id;
}

} // namespace world
