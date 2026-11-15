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
// exists yet, so it borrows it; uniqueness and the headquarters rule come from its flags.
world::Construction mine() {
    return {"Test Mine", "icon_test", world::BuildingType::Headquarters,
            {{world::items::IronOre, 10}}, 5.0f, false, true};
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

TEST(BuildRules, TheHeadquartersIsFreeInstantAndUnique) {
    // Given / When
    const world::Construction hq = headquarters();

    // Then
    EXPECT_TRUE(hq.cost.empty());
    EXPECT_FLOAT_EQ(hq.effort, 0.0f);
    EXPECT_TRUE(hq.unique);
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
