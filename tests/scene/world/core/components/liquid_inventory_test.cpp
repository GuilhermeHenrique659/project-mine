#include <gtest/gtest.h>

#include "scene/world/core/components/liquid_inventory.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/items/item.hpp"

namespace items = world::items;
using world::ItemCategory;
using world::ItemType;
using world::LiquidInventory;
using world::SolidInventory;

TEST(ItemCategory, OilAndDieselAreLiquidsAndTheOtherItemsAreSolid) {
    // Given
    const ItemType solids[] = {ItemType::IronOre, ItemType::CopperOre, ItemType::Coal,
                               ItemType::Steel,   ItemType::Electronics, ItemType::Tool,
                               ItemType::Ammo,    ItemType::Weapon};

    // When / Then
    for (ItemType type : solids) {
        EXPECT_EQ(world::item_of(type).category, ItemCategory::Solid) << world::to_string(type);
    }
    EXPECT_EQ(world::item_of(ItemType::Oil).category, ItemCategory::Liquid);
    EXPECT_EQ(world::item_of(ItemType::Diesel).category, ItemCategory::Liquid);
}

TEST(SolidInventory, RefusesLiquids) {
    // Given
    SolidInventory inventory{4};

    // When
    const int leftover = inventory.add(items::Oil, 10);

    // Then
    EXPECT_EQ(leftover, 10);
    EXPECT_TRUE(inventory.is_empty());
}

TEST(LiquidInventory, StoresALiquidUpToItsCapacityAndReturnsTheOverflow) {
    // Given
    LiquidInventory tank{500};

    // When
    const int leftover = tank.add(items::Oil, 600);

    // Then
    EXPECT_EQ(leftover, 100);
    EXPECT_EQ(tank.count(items::Oil), 500);
    EXPECT_EQ(tank.liquid(), ItemType::Oil);
}

TEST(LiquidInventory, RefusesSolidItems) {
    // Given
    LiquidInventory tank{500};

    // When
    const int leftover = tank.add(items::Coal, 10);

    // Then
    EXPECT_EQ(leftover, 10);
    EXPECT_TRUE(tank.is_empty());
}

TEST(LiquidInventory, RefusesASecondLiquidWhileHoldingOne) {
    // Given
    LiquidInventory tank{500};
    tank.add(items::Oil, 50);

    // When
    const int leftover = tank.add(items::Diesel, 20);

    // Then
    EXPECT_EQ(leftover, 20);
    EXPECT_EQ(tank.count(items::Diesel), 0);
    EXPECT_EQ(tank.count(items::Oil), 50);
}

TEST(LiquidInventory, EmptyingTheTankLetsItTakeAnotherLiquid) {
    // Given
    LiquidInventory tank{500};
    tank.add(items::Oil, 50);

    // When
    const int removed = tank.remove(items::Oil, 80);
    const int leftover = tank.add(items::Diesel, 20);

    // Then
    EXPECT_EQ(removed, 50);
    EXPECT_EQ(leftover, 0);
    EXPECT_EQ(tank.liquid(), ItemType::Diesel);
}

TEST(LiquidInventory, TransferMovesOnlyWhatTheDestinationTankHoldsRoomFor) {
    // Given
    LiquidInventory from{500};
    LiquidInventory to{100};
    from.add(items::Diesel, 300);

    // When
    const int moved = world::transfer_items(from, to, items::Diesel, 300);

    // Then
    EXPECT_EQ(moved, 100);
    EXPECT_EQ(from.count(items::Diesel), 200);
    EXPECT_EQ(to.count(items::Diesel), 100);
}
