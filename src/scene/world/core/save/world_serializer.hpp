#pragma once

#include <memory>
#include <vector>

#include "core/registry.hpp"
#include "game/save_game.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/save/component_codec.hpp"

namespace world {

// Turns the persistent entities of a registry into save records and back. It joins the
// factory (how to spawn an archetype) and the codecs (which data each component saves).
class WorldSerializer {
public:
    // The factory must outlive the serializer.
    WorldSerializer(const EntityFactory& factory,
                    std::vector<std::unique_ptr<ComponentCodec>> codecs);

    // A record for every entity with Persistent, in registry order.
    std::vector<game::EntityRecord> snapshot(const core::Registry& registry) const;

    // Spawns each record through the factory (skipping unknown archetypes), then lets
    // every codec apply the saved data.
    void restore(core::Registry& registry, const std::vector<game::EntityRecord>& records) const;

private:
    const EntityFactory& factory_;
    std::vector<std::unique_ptr<ComponentCodec>> codecs_;
};

// The serializer of the game: one codec per saved component.
WorldSerializer make_world_serializer(const EntityFactory& factory);

} // namespace world
