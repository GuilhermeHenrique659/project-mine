#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/systems/character_move_system.hpp"

TEST(Character, SpawnedCharacterWalksAtCharacterSpeedWhenGivenADirection) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = world::spawn_character(registry, "Hero", {2.0f, 1.0f});
    registry.get<world::Mover>(id)->direction = {0.0f, 1.0f};

    // When
    system.update(0.5f);

    // Then
    ASSERT_NE(registry.find(id), nullptr);
    EXPECT_EQ(registry.find(id)->label, "Hero");
    const world::Position* position = registry.get<world::Position>(id);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->point.x, 2.0f);
    EXPECT_FLOAT_EQ(position->point.y, 1.0f + world::CharacterSpeed * 0.5f);
}
