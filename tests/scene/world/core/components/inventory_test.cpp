#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/items/item.hpp"

namespace {

using world::Inventory;
using world::Item;
using world::ItemType;

int stack_count(const Inventory& inventory, std::size_t slot) {
    const auto& stack = inventory.slots()[slot];
    return stack ? stack->count : 0;
}

} // namespace

TEST(Inventory, AddingMoreThanOneStackSplitsAcrossSlots) {
    // Given
    Inventory inventory{5};

    // When
    const int leftover = inventory.add(world::items::IronOre, 150);

    // Then
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(stack_count(inventory, 0), 100);
    EXPECT_EQ(stack_count(inventory, 1), 50);
    EXPECT_EQ(inventory.free_slots(), 3u);
}

TEST(Inventory, AddingTopsUpAPartialStackBeforeUsingANewSlot) {
    // Given
    Inventory inventory{5};
    inventory.add(world::items::IronOre, 50);

    // When
    const int leftover = inventory.add(world::items::IronOre, 70);

    // Then
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(stack_count(inventory, 0), 100);
    EXPECT_EQ(stack_count(inventory, 1), 20);
}

TEST(Inventory, DifferentItemsNeverShareASlot) {
    // Given
    Inventory inventory{3};
    inventory.add(world::items::IronOre, 10);

    // When
    inventory.add(world::items::Coal, 10);

    // Then
    EXPECT_EQ(inventory.free_slots(), 1u);
    EXPECT_EQ(inventory.count(world::items::IronOre), 10);
    EXPECT_EQ(inventory.count(world::items::Coal), 10);
}

TEST(Inventory, ItemWithSmallerMaxStackSplitsAtItsOwnLimit) {
    // Given
    Inventory inventory{3};
    const Item small_coal{ItemType::Coal, 50};

    // When
    inventory.add(small_coal, 120);

    // Then
    EXPECT_EQ(stack_count(inventory, 0), 50);
    EXPECT_EQ(stack_count(inventory, 1), 50);
    EXPECT_EQ(stack_count(inventory, 2), 20);
}

TEST(Inventory, AddingToAFullInventoryReturnsTheWholeCount) {
    // Given
    Inventory inventory{1};
    inventory.add(world::items::IronOre, 100);

    // When
    const int leftover = inventory.add(world::items::IronOre, 30);

    // Then
    EXPECT_EQ(leftover, 30);
    EXPECT_EQ(inventory.count(world::items::IronOre), 100);
}

TEST(Inventory, AddingMoreThanFitsReturnsOnlyTheExcess) {
    // Given
    Inventory inventory{1};

    // When
    const int leftover = inventory.add(world::items::CopperOre, 150);

    // Then
    EXPECT_EQ(leftover, 50);
    EXPECT_EQ(inventory.count(world::items::CopperOre), 100);
}

TEST(Inventory, AddingZeroOrNegativeChangesNothing) {
    // Given
    Inventory inventory{2};

    // When
    const int zero = inventory.add(world::items::IronOre, 0);
    const int negative = inventory.add(world::items::IronOre, -5);

    // Then
    EXPECT_EQ(zero, 0);
    EXPECT_EQ(negative, 0);
    EXPECT_TRUE(inventory.is_empty());
}

TEST(Inventory, ItemWithNoStackSizeCannotBeAdded) {
    // Given
    Inventory inventory{2};

    // When
    const int leftover = inventory.add(Item{ItemType::Coal, 0}, 10);

    // Then
    EXPECT_EQ(leftover, 10);
    EXPECT_TRUE(inventory.is_empty());
}

TEST(Inventory, CountSumsTheItemAcrossSlots) {
    // Given
    Inventory inventory{4};
    inventory.add(world::items::IronOre, 130);
    inventory.add(world::items::Coal, 40);

    // When
    const int iron = inventory.count(world::items::IronOre);

    // Then
    EXPECT_EQ(iron, 130);
}

TEST(Inventory, RemovingMoreThanHeldRemovesOnlyWhatExistsAndFreesSlots) {
    // Given
    Inventory inventory{3};
    inventory.add(world::items::IronOre, 150);

    // When
    const int removed = inventory.remove(world::items::IronOre, 500);

    // Then
    EXPECT_EQ(removed, 150);
    EXPECT_TRUE(inventory.is_empty());
    EXPECT_EQ(inventory.free_slots(), 3u);
}

TEST(Inventory, RemovingTakesFromThePartialStackFirst) {
    // Given
    Inventory inventory{3};
    inventory.add(world::items::IronOre, 150);

    // When
    const int removed = inventory.remove(world::items::IronOre, 60);

    // Then
    EXPECT_EQ(removed, 60);
    EXPECT_EQ(stack_count(inventory, 0), 90);
    EXPECT_FALSE(inventory.slots()[1].has_value());
}

TEST(Inventory, RemovingZeroOrAnItemNotHeldRemovesNothing) {
    // Given
    Inventory inventory{2};
    inventory.add(world::items::IronOre, 10);

    // When
    const int zero = inventory.remove(world::items::IronOre, 0);
    const int missing = inventory.remove(world::items::Coal, 5);

    // Then
    EXPECT_EQ(zero, 0);
    EXPECT_EQ(missing, 0);
    EXPECT_EQ(inventory.count(world::items::IronOre), 10);
}

TEST(Inventory, EntitiesInARegistryKeepIndependentInventories) {
    // Given
    core::Registry registry;
    const core::EntityId chest = registry.create("Chest");
    const core::EntityId player = registry.create("Player");
    registry.add(chest, Inventory{4});
    registry.add(player, Inventory{4});

    // When
    registry.get<Inventory>(chest)->add(world::items::IronOre, 80);

    // Then
    EXPECT_EQ(registry.get<Inventory>(chest)->count(world::items::IronOre), 80);
    EXPECT_TRUE(registry.get<Inventory>(player)->is_empty());
}
