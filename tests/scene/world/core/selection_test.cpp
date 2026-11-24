#include <gtest/gtest.h>

#include <algorithm>

#include "core/registry.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/selection.hpp"
#include "scene/world/core/entities/battle_tank.hpp"
#include "scene/world/core/entities/vehicle.hpp"

namespace {

const world::IsoProjection Projection{};

world::ScreenPoint on_screen(world::WorldPoint point) { return Projection.world_to_screen(point); }

bool contains(const std::vector<world::TileCoord>& tiles, world::TileCoord tile) {
    return std::find(tiles.begin(), tiles.end(), tile) != tiles.end();
}

} // namespace

TEST(Selection, ABoxSelectsOnlyTheSoldiersDrawnInsideIt) {
    // Given
    core::Registry registry;
    const core::EntityId inside = world::spawn_soldier(registry, "Soldier", {2.0f, 2.0f});
    world::spawn_soldier(registry, "Soldier", {10.0f, 2.0f});
    const world::ScreenPoint at = on_screen({2.0f, 2.0f});

    // When
    const auto selected =
        world::soldiers_in(registry, Projection, {at.x - 10, at.y - 10}, {at.x + 10, at.y + 10});

    // Then
    EXPECT_EQ(selected, std::vector<core::EntityId>{inside});
}

TEST(Selection, TheBoxCornersCanBeGivenInAnyOrder) {
    // Given
    core::Registry registry;
    const core::EntityId soldier = world::spawn_soldier(registry, "Soldier", {2.0f, 2.0f});
    const world::ScreenPoint at = on_screen({2.0f, 2.0f});

    // When: dragged from bottom right to top left
    const auto selected =
        world::soldiers_in(registry, Projection, {at.x + 10, at.y + 10}, {at.x - 10, at.y - 10});

    // Then
    EXPECT_EQ(selected, std::vector<core::EntityId>{soldier});
}

TEST(Selection, WorkersInsideTheBoxAreNotSelected) {
    // Given
    core::Registry registry;
    world::spawn_worker(registry, "Worker", {2.0f, 2.0f});
    const world::ScreenPoint at = on_screen({2.0f, 2.0f});

    // When
    const auto selected =
        world::soldiers_in(registry, Projection, {at.x - 10, at.y - 10}, {at.x + 10, at.y + 10});

    // Then
    EXPECT_TRUE(selected.empty());
}

TEST(Selection, AFormationGivesEachUnitItsOwnTileStartingOnTheGoal) {
    // Given
    const auto nothing_blocked = [](world::TileCoord) { return false; };

    // When
    const auto tiles = world::formation_tiles({5, 5}, 4, nothing_blocked);

    // Then
    ASSERT_EQ(tiles.size(), 4u);
    EXPECT_EQ(tiles.front(), (world::TileCoord{5, 5}));
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        EXPECT_LE(std::max(std::abs(tiles[i].x - 5), std::abs(tiles[i].y - 5)), 1);
        for (std::size_t j = i + 1; j < tiles.size(); ++j) {
            EXPECT_FALSE(tiles[i] == tiles[j]);
        }
    }
}

TEST(Selection, AFormationSkipsBlockedTilesIncludingTheGoal) {
    // Given
    const auto goal_blocked = [](world::TileCoord tile) { return tile == world::TileCoord{5, 5}; };

    // When
    const auto tiles = world::formation_tiles({5, 5}, 2, goal_blocked);

    // Then
    ASSERT_EQ(tiles.size(), 2u);
    EXPECT_FALSE(contains(tiles, {5, 5}));
}

TEST(Selection, AFormationWithNoFreeTileNearbyIsEmpty) {
    // Given
    const auto everything_blocked = [](world::TileCoord) { return true; };

    // When
    const auto tiles = world::formation_tiles({5, 5}, 3, everything_blocked);

    // Then
    EXPECT_TRUE(tiles.empty());
}

TEST(Selection, ABoxSelectsTanksButNotTheSoldiersInsideThem) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {2.0f, 2.0f});
    world::board(registry, tank, world::spawn_soldier(registry, "Crew", {2.0f, 2.0f}));
    const world::ScreenPoint at = on_screen({2.0f, 2.0f});

    // When
    const auto selected =
        world::soldiers_in(registry, Projection, {at.x - 10, at.y - 10}, {at.x + 10, at.y + 10});

    // Then
    EXPECT_EQ(selected, std::vector<core::EntityId>{tank});
}
