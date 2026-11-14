#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "logging/logger.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/persistent.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/save/world_serializer.hpp"

namespace {

struct Fixture {
    world::EntityFactory factory = world::make_entity_factory();
    world::WorldSerializer serializer = world::make_world_serializer(factory);
    core::Registry registry;

    core::EntityId spawn_player(world::WorldPoint at) {
        const core::EntityId id = *factory.spawn(registry, "character", "Player");
        registry.get<world::Position>(id)->point = at;
        registry.add(id, world::PlayerControlled{});
        return id;
    }
};

game::EntityRecord player_record(std::map<std::string, std::string> fields) {
    return {"character", "Player", std::move(fields)};
}

} // namespace

TEST(WorldSerializer, SnapshotOfAMovingPlayerWritesItsPositionAndIdle) {
    // Given
    Fixture f;
    const core::EntityId id = f.spawn_player({3.25f, -1.5f});
    f.registry.get<world::Character>(id)->state = world::CharacterState::Moving;

    // When
    const auto records = f.serializer.snapshot(f.registry);

    // Then
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].archetype, "character");
    EXPECT_EQ(records[0].label, "Player");
    EXPECT_EQ(records[0].fields.at("position.x"), "3.25");
    EXPECT_EQ(records[0].fields.at("position.y"), "-1.5");
    EXPECT_EQ(records[0].fields.at("state"), "Idle");
    EXPECT_EQ(records[0].fields.at("player"), "1");
}

TEST(WorldSerializer, RunningIsAlsoSavedAsIdle) {
    // Given
    Fixture f;
    const core::EntityId id = f.spawn_player({0.0f, 0.0f});
    f.registry.get<world::Character>(id)->state = world::CharacterState::Running;

    // When
    const auto records = f.serializer.snapshot(f.registry);

    // Then
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].fields.at("state"), "Idle");
}

TEST(WorldSerializer, EntitiesWithoutPersistentAreNotSaved) {
    // Given
    Fixture f;
    f.spawn_player({1.0f, 1.0f});
    const core::EntityId transient = f.registry.create("Spark");
    f.registry.add(transient, world::Position{{5.0f, 5.0f}});

    // When
    const auto records = f.serializer.snapshot(f.registry);

    // Then
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].label, "Player");
}

TEST(WorldSerializer, FractionalPositionsRoundTripExactly) {
    // Given
    Fixture f;
    f.spawn_player({0.1f, -123.456f});
    const auto records = f.serializer.snapshot(f.registry);
    core::Registry loaded;

    // When
    f.serializer.restore(loaded, records);

    // Then
    ASSERT_EQ(loaded.entities().size(), 1u);
    const core::EntityId id = loaded.entities()[0].id;
    EXPECT_EQ(loaded.get<world::Position>(id)->point.x, 0.1f);
    EXPECT_EQ(loaded.get<world::Position>(id)->point.y, -123.456f);
}

TEST(WorldSerializer, RestoreRecreatesPositionIdleStateAndPlayerMarker) {
    // Given
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{player_record(
        {{"position.x", "4"}, {"position.y", "-2"}, {"state", "Idle"}, {"player", "1"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then
    ASSERT_EQ(f.registry.entities().size(), 1u);
    const core::EntityId id = f.registry.entities()[0].id;
    EXPECT_EQ(f.registry.find(id)->label, "Player");
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.x, 4.0f);
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.y, -2.0f);
    EXPECT_EQ(f.registry.get<world::Character>(id)->state, world::CharacterState::Idle);
    EXPECT_TRUE(f.registry.has<world::PlayerControlled>(id));
    EXPECT_TRUE(f.registry.has<world::Persistent>(id));
}

TEST(WorldSerializer, RestoreSkipsAnUnknownArchetypeAndKeepsTheRest) {
    // Given
    Fixture f;
    logging::set_sink([](logging::Level, const std::string&) {});
    const auto records = std::vector<game::EntityRecord>{
        {"dragon", "Smaug", {}}, player_record({{"position.x", "2"}, {"position.y", "2"}})};

    // When
    f.serializer.restore(f.registry, records);
    logging::set_sink({});

    // Then
    ASSERT_EQ(f.registry.entities().size(), 1u);
    EXPECT_EQ(f.registry.entities()[0].label, "Player");
}

TEST(WorldSerializer, ABadPositionValueKeepsTheDefault) {
    // Given
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{
        player_record({{"position.x", "abc"}, {"position.y", "5"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then: neither coordinate is applied when one is invalid
    const core::EntityId id = f.registry.entities()[0].id;
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.x, 0.0f);
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.y, 0.0f);
}

TEST(WorldSerializer, AnUnknownStateRestoresAsIdle) {
    // Given
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{player_record({{"state", "Flying"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then
    const core::EntityId id = f.registry.entities()[0].id;
    EXPECT_EQ(f.registry.get<world::Character>(id)->state, world::CharacterState::Idle);
}

TEST(WorldSerializer, ASavedMovingStateRestoresAsIdle) {
    // Given: movement intent is not saved, so the character is standing after a load
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{player_record({{"state", "Moving"}}),
                                                         player_record({{"state", "Running"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then
    for (const core::Entity& entity : f.registry.entities()) {
        EXPECT_EQ(f.registry.get<world::Character>(entity.id)->state, world::CharacterState::Idle);
    }
}
