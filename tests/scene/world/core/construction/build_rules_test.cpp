#include <gtest/gtest.h>

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/construction/build_rules.hpp"

namespace {

constexpr world::TileCoord Grass{1, 1}; // inside the spawn clearing: always grass

world::Construction headquarters() {
    return *world::find_construction(world::BuildingType::Headquarters);
}

// A construction that needs a headquarters and costs 10 iron ore. Only one building type
// exists yet, so it borrows it; it is not unique and needs a headquarters (its rules).
world::Construction mine() {
    return {"Test Mine", "icon_test", world::BuildingType::Headquarters,
            {{world::items::IronOre, 10}}, 5.0f, {std::make_shared<world::NeedsHeadquartersRule>()}};
}

std::optional<world::TileCoord> find_water(const world::Terrain& terrain) {
    for (int x = 0; x < 400; ++x) {
        for (int y = 0; y < 400; ++y) {
            if (terrain.tile_at({x, y}) == world::TileType::Water) {
                return world::TileCoord{x, y};
            }
        }
    }
    return std::nullopt;
}

void place(core::Registry& registry, world::BuildingType type, world::TileCoord tile) {
    const core::EntityId id = registry.create("building");
    registry.add(id, world::Building{type, tile});
}

} // namespace

TEST(BuildRules, TheHeadquartersIsFreeAndInstant) {
    // Given / When
    const world::Construction hq = headquarters();

    // Then
    EXPECT_TRUE(hq.cost.empty());
    EXPECT_FLOAT_EQ(hq.effort, 0.0f);
}

TEST(BuildRules, TheFirstHeadquartersCanBeBuiltWithoutPaying) {
    // Given
    core::Registry registry;
    const world::Terrain terrain(7);

    // When
    const auto result = world::can_build(registry, headquarters(), Grass, terrain, nullptr);

    // Then
    EXPECT_EQ(result, world::BuildResult::Ok);
}

TEST(BuildRules, ASecondHeadquartersIsRefused) {
    // Given
    core::Registry registry;
    const world::Terrain terrain(7);
    place(registry, world::BuildingType::Headquarters, {5, 5});

    // When
    const auto result = world::can_build(registry, headquarters(), Grass, terrain, nullptr);

    // Then
    EXPECT_TRUE(world::has_headquarters(registry));
    EXPECT_EQ(result, world::BuildResult::AlreadyExists);
}

TEST(BuildRules, EntriesWithoutATypeCannotBeBuilt) {
    // Given
    core::Registry registry;
    place(registry, world::BuildingType::Headquarters, {5, 5});

    // When
    const auto result = world::check_unlocked(registry, world::Construction{"Storage", "icon"});

    // Then
    EXPECT_EQ(result, world::BuildResult::NotBuildable);
}

TEST(BuildRules, OtherBuildingsNeedAHeadquartersFirst) {
    // Given
    core::Registry registry;

    // When
    const auto before = world::check_unlocked(registry, mine());
    place(registry, world::BuildingType::Headquarters, {5, 5});
    const auto after = world::check_unlocked(registry, mine());

    // Then
    EXPECT_EQ(before, world::BuildResult::NeedsHeadquarters);
    EXPECT_EQ(after, world::BuildResult::Ok);
}

TEST(BuildRules, WaterAndOccupiedTilesAreBlocked) {
    // Given
    core::Registry registry;
    const world::Terrain terrain(7);
    const auto water = find_water(terrain);
    ASSERT_TRUE(water);
    place(registry, world::BuildingType::Headquarters, Grass);
    world::Inventory inventory(10);
    inventory.add(world::items::IronOre, 10);

    // When
    const auto on_water = world::can_build(registry, mine(), *water, terrain, &inventory);
    const auto on_building = world::can_build(registry, mine(), Grass, terrain, &inventory);

    // Then
    EXPECT_EQ(on_water, world::BuildResult::TileBlocked);
    EXPECT_EQ(on_building, world::BuildResult::TileBlocked);
}

TEST(BuildRules, TheCostMustBeInThePayersInventory) {
    // Given
    core::Registry registry;
    const world::Terrain terrain(7);
    place(registry, world::BuildingType::Headquarters, {5, 5});
    world::Inventory inventory(10);
    inventory.add(world::items::IronOre, 9);

    // When
    const auto short_by_one = world::can_build(registry, mine(), Grass, terrain, &inventory);
    inventory.add(world::items::IronOre, 1);
    const auto enough = world::can_build(registry, mine(), Grass, terrain, &inventory);
    const auto no_payer = world::can_build(registry, mine(), Grass, terrain, nullptr);

    // Then
    EXPECT_EQ(short_by_one, world::BuildResult::NotEnoughResources);
    EXPECT_EQ(enough, world::BuildResult::Ok);
    EXPECT_EQ(no_payer, world::BuildResult::NotEnoughResources);
}

namespace {

std::optional<world::TileCoord> find_ore(const world::Terrain& terrain) {
    for (int x = -60; x < 60; ++x) {
        for (int y = -60; y < 60; ++y) {
            if (world::is_ore(terrain.tile_at({x, y}))) {
                return world::TileCoord{x, y};
            }
        }
    }
    return std::nullopt;
}

world::Construction catalog_mine() { return *world::find_construction(world::BuildingType::Mine); }

} // namespace

TEST(BuildRules, AMineCosts100IronOreAnd300Effort) {
    // Given / When
    const world::Construction mine = catalog_mine();

    // Then
    ASSERT_EQ(mine.cost.size(), 1u);
    EXPECT_EQ(mine.cost[0].item, world::items::IronOre);
    EXPECT_EQ(mine.cost[0].count, 100);
    EXPECT_FLOAT_EQ(mine.effort, 300.0f);
}

TEST(BuildRules, AMineMustStandOnAnOreTile) {
    // Given
    core::Registry registry;
    const world::Terrain terrain(7);
    place(registry, world::BuildingType::Headquarters, {5, 5});
    world::Inventory inventory(10);
    inventory.add(world::items::IronOre, 100);
    const auto ore = find_ore(terrain);
    ASSERT_TRUE(ore);

    // When
    const auto on_grass = world::can_build(registry, catalog_mine(), Grass, terrain, &inventory);
    const auto on_ore = world::can_build(registry, catalog_mine(), *ore, terrain, &inventory);

    // Then
    EXPECT_EQ(on_grass, world::BuildResult::NeedsOre);
    EXPECT_EQ(on_ore, world::BuildResult::Ok);
}

TEST(BuildRules, AMultiTileBuildingIsBlockedByABuildingUnderAnyOfItsTiles) {
    // Given
    core::Registry registry;
    world::Terrain terrain(7);
    place(registry, world::BuildingType::Headquarters, {Grass.x + 1, Grass.y + 1});
    world::Construction big = headquarters();
    big.rules.clear();
    big.size = {2, 2};

    // When
    const auto overlapping = world::can_build(registry, big, Grass, terrain, nullptr);
    const auto beside = world::can_build(registry, big, {Grass.x - 2, Grass.y}, terrain, nullptr);

    // Then
    EXPECT_EQ(overlapping, world::BuildResult::TileBlocked);
    EXPECT_EQ(beside, world::BuildResult::Ok);
}

TEST(BuildRules, AMultiTileBuildingIsBlockedByWaterUnderAnyOfItsTiles) {
    // Given
    core::Registry registry;
    world::Terrain terrain(7);
    std::optional<world::TileCoord> origin;
    for (int x = 0; x < 400 && !origin; ++x) {
        for (int y = 0; y < 400 && !origin; ++y) {
            if (terrain.tile_at({x, y}) == world::TileType::Grass &&
                terrain.tile_at({x + 1, y + 1}) == world::TileType::Water) {
                origin = world::TileCoord{x, y};
            }
        }
    }
    ASSERT_TRUE(origin);
    world::Construction big = headquarters();
    big.rules.clear();
    big.size = {2, 2};

    // When: the origin is dry, the footprint's far corner is water
    const auto result = world::can_build(registry, big, *origin, terrain, nullptr);

    // Then
    EXPECT_EQ(result, world::BuildResult::TileBlocked);
}
