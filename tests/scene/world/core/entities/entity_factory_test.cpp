#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/components/persistent.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/entities/entity_factory.hpp"

TEST(EntityFactory, SpawningARegisteredArchetypeAddsItsComponentsAndPersistent) {
    // Given
    core::Registry registry;
    world::EntityFactory factory;
    factory.register_archetype("scout", [](core::Registry& r, const std::string& label) {
        return world::spawn_character(r, label, {1.0f, 2.0f});
    });

    // When
    const auto id = factory.spawn(registry, "scout", "Ann");

    // Then
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(registry.find(*id)->label, "Ann");
    ASSERT_NE(registry.get<world::Position>(*id), nullptr);
    EXPECT_FLOAT_EQ(registry.get<world::Position>(*id)->point.x, 1.0f);
    ASSERT_NE(registry.get<world::Persistent>(*id), nullptr);
    EXPECT_EQ(registry.get<world::Persistent>(*id)->archetype, "scout");
}

TEST(EntityFactory, SpawningAnUnknownArchetypeCreatesNothing) {
    // Given
    core::Registry registry;
    const world::EntityFactory factory;

    // When
    const auto id = factory.spawn(registry, "dragon", "Smaug");

    // Then
    EXPECT_FALSE(id.has_value());
    EXPECT_TRUE(registry.entities().empty());
}

TEST(EntityFactory, RegisteringTheSameNameAgainReplacesTheSpawner) {
    // Given
    core::Registry registry;
    world::EntityFactory factory;
    factory.register_archetype("unit", [](core::Registry& r, const std::string& label) {
        return world::spawn_character(r, label, {1.0f, 0.0f});
    });
    factory.register_archetype("unit", [](core::Registry& r, const std::string& label) {
        return world::spawn_character(r, label, {9.0f, 0.0f});
    });

    // When
    const auto id = factory.spawn(registry, "unit", "U");

    // Then
    ASSERT_TRUE(id.has_value());
    EXPECT_FLOAT_EQ(registry.get<world::Position>(*id)->point.x, 9.0f);
}
