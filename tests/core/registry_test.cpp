#include <gtest/gtest.h>

#include <vector>

#include "core/registry.hpp"

namespace {

struct Health {
    int value = 0;
};

struct Armor {
    int value = 0;
};

} // namespace

TEST(Registry, CreatedEntitiesGetDistinctIdsAndKeepTheirLabel) {
    // Given
    core::Registry registry;

    // When
    const core::EntityId first = registry.create("Miner");
    const core::EntityId second = registry.create("Belt");

    // Then
    EXPECT_NE(first, second);
    ASSERT_NE(registry.find(first), nullptr);
    ASSERT_NE(registry.find(second), nullptr);
    EXPECT_EQ(registry.find(first)->label, "Miner");
    EXPECT_EQ(registry.find(second)->label, "Belt");
}

TEST(Registry, EachVisitsOnlyEntitiesThatHaveAllTheRequestedComponents) {
    // Given
    core::Registry registry;
    const core::EntityId both = registry.create("both");
    const core::EntityId only_health = registry.create("only health");
    registry.add(both, Health{10});
    registry.add(both, Armor{3});
    registry.add(only_health, Health{5});
    std::vector<core::EntityId> visited;

    // When
    registry.each<Health, Armor>([&](core::EntityId id, Health& health, Armor& armor) {
        visited.push_back(id);
        health.value += armor.value;
    });

    // Then
    ASSERT_EQ(visited.size(), 1u);
    EXPECT_EQ(visited.front(), both);
    EXPECT_EQ(registry.get<Health>(both)->value, 13);
    EXPECT_EQ(registry.get<Health>(only_health)->value, 5);
}

TEST(Registry, DestroyedEntityLosesItsComponentsAndIsNoLongerVisited) {
    // Given
    core::Registry registry;
    const core::EntityId doomed = registry.create("doomed");
    const core::EntityId survivor = registry.create("survivor");
    registry.add(doomed, Health{1});
    registry.add(survivor, Health{2});
    int visits = 0;

    // When
    registry.destroy(doomed);
    registry.each<Health>([&](core::EntityId, Health&) { ++visits; });

    // Then
    EXPECT_FALSE(registry.is_alive(doomed));
    EXPECT_EQ(registry.find(doomed), nullptr);
    EXPECT_FALSE(registry.has<Health>(doomed));
    EXPECT_EQ(registry.get<Health>(doomed), nullptr);
    EXPECT_TRUE(registry.is_alive(survivor));
    EXPECT_EQ(visits, 1);
}

TEST(Registry, IdsAreNotReusedAfterDestroy) {
    // Given
    core::Registry registry;
    const core::EntityId first = registry.create("first");
    registry.destroy(first);

    // When
    const core::EntityId second = registry.create("second");

    // Then
    EXPECT_NE(first, second);
    EXPECT_FALSE(registry.is_alive(first));
}

TEST(Registry, RemovingAComponentKeepsTheEntityAndItsOtherComponents) {
    // Given
    core::Registry registry;
    const core::EntityId id = registry.create("unit");
    registry.add(id, Health{4});
    registry.add(id, Armor{2});

    // When
    registry.remove<Armor>(id);

    // Then
    EXPECT_TRUE(registry.is_alive(id));
    EXPECT_FALSE(registry.has<Armor>(id));
    EXPECT_TRUE(registry.has<Health>(id));
}
