#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/construction/blueprint.hpp"

TEST(Blueprint, ApplyGivesTheEntityEveryComponent) {
    // Given
    core::Registry registry;
    const core::EntityId id = registry.create("Mine");
    const world::Blueprint blueprint =
        world::Blueprint{}.with(world::Inventory{10}).with(world::PowerConsumer{100.0f});

    // When
    blueprint.apply(registry, id);

    // Then
    ASSERT_NE(registry.get<world::Inventory>(id), nullptr);
    EXPECT_EQ(registry.get<world::Inventory>(id)->capacity(), 10u);
    ASSERT_NE(registry.get<world::PowerConsumer>(id), nullptr);
    EXPECT_FLOAT_EQ(registry.get<world::PowerConsumer>(id)->demand_kw, 100.0f);
}

TEST(Blueprint, ApplyKeepsAComponentTheEntityAlreadyHas) {
    // Given: a loaded mine whose inventory already holds coal
    core::Registry registry;
    const core::EntityId id = registry.create("Mine");
    registry.add(id, world::Inventory{10}).add(world::items::Coal, 40);
    const world::Blueprint blueprint = world::Blueprint{}.with(world::Inventory{10});

    // When
    blueprint.apply(registry, id);

    // Then
    EXPECT_EQ(registry.get<world::Inventory>(id)->count(world::items::Coal), 40);
}

TEST(Blueprint, FindGivesTheComponentDataOrNullWhenAbsent) {
    // Given
    const world::Blueprint blueprint = world::Blueprint{}.with(world::PowerConsumer{100.0f});

    // When
    const world::PowerConsumer* consumer = blueprint.find<world::PowerConsumer>();
    const world::Generator* generator = blueprint.find<world::Generator>();

    // Then
    ASSERT_NE(consumer, nullptr);
    EXPECT_FLOAT_EQ(consumer->demand_kw, 100.0f);
    EXPECT_EQ(generator, nullptr);
}
