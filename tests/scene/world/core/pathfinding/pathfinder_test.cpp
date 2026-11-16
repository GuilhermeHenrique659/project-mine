#include <gtest/gtest.h>

#include <cstdlib>
#include <set>
#include <utility>

#include "scene/world/core/pathfinding/pathfinder.hpp"

namespace {

using world::TileCoord;

// A set of blocked tiles standing in for water and buildings.
struct Walls {
    std::set<std::pair<int, int>> tiles;

    void block(TileCoord tile) { tiles.insert({tile.x, tile.y}); }

    world::BlockedFn fn() const {
        return [this](TileCoord tile) { return tiles.contains({tile.x, tile.y}); };
    }
};

bool is_connected(const std::vector<TileCoord>& path) {
    for (std::size_t i = 1; i < path.size(); ++i) {
        const int dx = std::abs(path[i].x - path[i - 1].x);
        const int dy = std::abs(path[i].y - path[i - 1].y);
        if (dx > 1 || dy > 1 || (dx == 0 && dy == 0)) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST(Pathfinder, OpenGroundGivesAStraightPathFromStartToGoal) {
    // Given
    Walls walls;

    // When
    const auto path = world::find_path({0, 0}, {4, 0}, walls.fn());

    // Then
    ASSERT_TRUE(path.has_value());
    const std::vector<TileCoord> expected{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
    EXPECT_EQ(*path, expected);
}

TEST(Pathfinder, DiagonalStepsShortenThePath) {
    // Given
    Walls walls;

    // When
    const auto path = world::find_path({0, 0}, {3, 3}, walls.fn());

    // Then
    ASSERT_TRUE(path.has_value());
    EXPECT_EQ(path->size(), 4u);
}

TEST(Pathfinder, PathGoesAroundAWallAndNeverStepsOnIt) {
    // Given: a wall from (2,-3) to (2,3) between start and goal
    Walls walls;
    for (int y = -3; y <= 3; ++y) {
        walls.block({2, y});
    }

    // When
    const auto path = world::find_path({0, 0}, {4, 0}, walls.fn());

    // Then
    ASSERT_TRUE(path.has_value());
    EXPECT_TRUE(is_connected(*path));
    for (const TileCoord tile : *path) {
        EXPECT_FALSE(walls.fn()(tile)) << tile.x << "," << tile.y;
    }
    EXPECT_EQ(path->front(), (TileCoord{0, 0}));
    EXPECT_EQ(path->back(), (TileCoord{4, 0}));
}

TEST(Pathfinder, DiagonalStepDoesNotCutTheCornerOfABlockedTile) {
    // Given: (1,0) is blocked, so (0,0) -> (1,1) would squeeze past its corner
    Walls walls;
    walls.block({1, 0});

    // When
    const auto path = world::find_path({0, 0}, {1, 1}, walls.fn());

    // Then
    ASSERT_TRUE(path.has_value());
    const std::vector<TileCoord> expected{{0, 0}, {0, 1}, {1, 1}};
    EXPECT_EQ(*path, expected);
}

TEST(Pathfinder, BlockedStartAndGoalAreStillUsedAsEndpoints) {
    // Given: both endpoints are buildings
    Walls walls;
    walls.block({0, 0});
    walls.block({3, 0});

    // When
    const auto path = world::find_path({0, 0}, {3, 0}, walls.fn());

    // Then
    ASSERT_TRUE(path.has_value());
    EXPECT_EQ(path->size(), 4u);
}

TEST(Pathfinder, EnclosedGoalHasNoPath) {
    // Given: the goal (10,10) is walled in on all eight sides
    Walls walls;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx != 0 || dy != 0) {
                walls.block({10 + dx, 10 + dy});
            }
        }
    }

    // When
    const auto path = world::find_path({0, 0}, {10, 10}, walls.fn(), 2000);

    // Then
    EXPECT_FALSE(path.has_value());
}

TEST(Pathfinder, SameStartAndGoalIsASingleTilePath) {
    // Given
    Walls walls;

    // When
    const auto path = world::find_path({5, 5}, {5, 5}, walls.fn());

    // Then
    ASSERT_TRUE(path.has_value());
    EXPECT_EQ(path->size(), 1u);
}
