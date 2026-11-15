#include "scene/world/core/save/world_serializer.hpp"

#include "logging/logger.hpp"
#include "scene/world/core/components/persistent.hpp"
#include "scene/world/core/save/codecs.hpp"

namespace world {

WorldSerializer::WorldSerializer(const EntityFactory& factory,
                                 std::vector<std::unique_ptr<ComponentCodec>> codecs)
    : factory_(factory), codecs_(std::move(codecs)) {}

std::vector<game::EntityRecord> WorldSerializer::snapshot(const core::Registry& registry) const {
    std::vector<game::EntityRecord> records;
    for (const core::Entity& entity : registry.entities()) {
        const Persistent* persistent = registry.get<Persistent>(entity.id);
        if (persistent == nullptr) {
            continue;
        }
        game::EntityRecord record{persistent->archetype, entity.label, {}};
        for (const auto& codec : codecs_) {
            codec->write(registry, entity.id, record.fields);
        }
        records.push_back(std::move(record));
    }
    return records;
}

void WorldSerializer::restore(core::Registry& registry,
                              const std::vector<game::EntityRecord>& records) const {
    for (const game::EntityRecord& record : records) {
        // Defaults first, then the saved data on top.
        const auto id = factory_.spawn(registry, record.archetype, record.label);
        if (!id) {
            logging::warning("Skipping saved entity with unknown archetype \"" +
                             record.archetype + "\"");
            continue;
        }
        for (const auto& codec : codecs_) {
            codec->read(registry, *id, record.fields);
        }
    }
}

WorldSerializer make_world_serializer(const EntityFactory& factory) {
    std::vector<std::unique_ptr<ComponentCodec>> codecs;
    codecs.push_back(std::make_unique<PositionCodec>());
    codecs.push_back(std::make_unique<CharacterCodec>());
    codecs.push_back(std::make_unique<PlayerControlledCodec>());
    codecs.push_back(std::make_unique<InventoryCodec>());
    // Building first: it re-places the building, then the site restores its progress.
    codecs.push_back(std::make_unique<BuildingCodec>());
    codecs.push_back(std::make_unique<ConstructionSiteCodec>());
    return WorldSerializer(factory, std::move(codecs));
}

} // namespace world
