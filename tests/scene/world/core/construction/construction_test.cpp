#include <gtest/gtest.h>

#include <set>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/worker.hpp"
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

TEST(ConstructionCatalog, SteelworksCosts300IronOreTakes1200WorkNeeds50WorkersOn200Kw) {
    // Given / When
    const world::Construction* steelworks =
        world::find_construction(world::BuildingType::Steelworks);

    // Then
    ASSERT_NE(steelworks, nullptr);
    EXPECT_EQ(steelworks->name, "Steelworks");
    ASSERT_EQ(steelworks->cost.size(), 1u);
    EXPECT_EQ(steelworks->cost[0].item, world::items::IronOre);
    EXPECT_EQ(steelworks->cost[0].count, 300);
    EXPECT_FLOAT_EQ(steelworks->effort, 1200.0f);
    const auto* staffing = steelworks->blueprint.find<world::Staffing>();
    ASSERT_NE(staffing, nullptr);
    EXPECT_EQ(staffing->needed, 50);
    const auto* power = steelworks->blueprint.find<world::PowerConsumer>();
    ASSERT_NE(power, nullptr);
    EXPECT_FLOAT_EQ(power->demand_kw, 200.0f);
}

TEST(ConstructionCatalog, SteelworksMakesOneSteelFromTwoCoalAndTwoIronOreAtTwoPerSecond) {
    // Given / When
    const auto* producer = world::find_construction(world::BuildingType::Steelworks)
                               ->blueprint.find<world::Producer>();

    // Then
    ASSERT_NE(producer, nullptr);
    const world::Recipe& recipe = producer->recipe;
    EXPECT_EQ(recipe.output, world::items::Steel);
    EXPECT_FLOAT_EQ(recipe.rate, 2.0f);
    ASSERT_EQ(recipe.inputs.size(), 2u);
    EXPECT_EQ(recipe.inputs[0].item, world::items::IronOre);
    EXPECT_EQ(recipe.inputs[0].count, 2);
    EXPECT_EQ(recipe.inputs[1].item, world::items::Coal);
    EXPECT_EQ(recipe.inputs[1].count, 2);
}

TEST(BuildingType, SteelworksRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::Steelworks);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "Steelworks");
    EXPECT_EQ(parsed, world::BuildingType::Steelworks);
}

TEST(ConstructionCatalog, SteelworksHasGeneralSlotsAndOneSectionForIronOreAndOneForCoal) {
    // Given
    world::Inventory inventory =
        *world::find_construction(world::BuildingType::Steelworks)->blueprint.find<world::Inventory>();

    // When
    const int iron_left = inventory.add(world::items::IronOre, 1000);
    const int coal_left = inventory.add(world::items::Coal, 1000);
    const int steel_left = inventory.add(world::items::Steel, 2000);

    // Then: 5 slots each for iron and coal, 10 general slots for the steel
    EXPECT_EQ(inventory.capacity(), 20u);
    EXPECT_EQ(iron_left, 500);
    EXPECT_EQ(coal_left, 500);
    EXPECT_EQ(steel_left, 1000);
}
