#include <gtest/gtest.h>

#include <set>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/construction/construction.hpp"

TEST(ConstructionCatalog, HasSpecialMinesFactoriesAndMilitaryInThatOrder) {
    // Given / When
    const auto& catalog = world::construction_catalog();

    // Then
    ASSERT_EQ(catalog.size(), 4u);
    EXPECT_EQ(catalog[0].category, world::ConstructionCategory::Special);
    EXPECT_EQ(catalog[1].category, world::ConstructionCategory::Mine);
    EXPECT_EQ(catalog[2].category, world::ConstructionCategory::Factory);
    EXPECT_EQ(catalog[3].category, world::ConstructionCategory::Military);
}

TEST(ConstructionCatalog, EveryCategoryOffersConstructionsWithTheirOwnIcons) {
    // Given
    const auto& catalog = world::construction_catalog();
    std::set<std::string> icons;
    std::size_t total = 0;

    // When
    for (const auto& group : catalog) {
        EXPECT_FALSE(group.constructions.empty()) << group.name;
        icons.insert(group.icon);
        ++total;
        for (const auto& construction : group.constructions) {
            icons.insert(construction.icon);
            ++total;
        }
    }

    // Then
    EXPECT_EQ(icons.size(), total);
}

TEST(ConstructionCatalog, StorageCosts200IronOreTakes600WorkAndHolds120SlotsOn20Kw) {
    // Given / When
    const world::Construction* storage = world::find_construction(world::BuildingType::Storage);

    // Then
    ASSERT_NE(storage, nullptr);
    EXPECT_EQ(storage->name, "Storage");
    ASSERT_EQ(storage->cost.size(), 1u);
    EXPECT_EQ(storage->cost[0].item, world::items::IronOre);
    EXPECT_EQ(storage->cost[0].count, 200);
    EXPECT_FLOAT_EQ(storage->effort, 600.0f);
    const auto* inventory = storage->blueprint.find<world::Inventory>();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->capacity(), 120u);
    const auto* power = storage->blueprint.find<world::PowerConsumer>();
    ASSERT_NE(power, nullptr);
    EXPECT_FLOAT_EQ(power->demand_kw, 20.0f);
}

TEST(BuildingType, StorageRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::Storage);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "Storage");
    EXPECT_EQ(parsed, world::BuildingType::Storage);
}
