#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "world/isometric.hpp"

namespace {

// 64x32 tiles with the camera scrolled so world (0, 0) is at (400, 100).
world::IsoProjection scrolled_projection() {
    world::IsoProjection projection;
    projection.origin = {400.0f, 100.0f};
    return projection;
}

} // namespace

TEST(IsoProjection, WorldXAxisGoesRightAndDownOnScreen) {
    // Given
    const world::IsoProjection projection = scrolled_projection();

    // When
    const world::ScreenPoint screen = projection.world_to_screen({1.0f, 0.0f});

    // Then
    EXPECT_FLOAT_EQ(screen.x, 400.0f + 32.0f);
    EXPECT_FLOAT_EQ(screen.y, 100.0f + 16.0f);
}

TEST(IsoProjection, WorldYAxisGoesLeftAndDownOnScreen) {
    // Given
    const world::IsoProjection projection = scrolled_projection();

    // When
    const world::ScreenPoint screen = projection.world_to_screen({0.0f, 1.0f});

    // Then
    EXPECT_FLOAT_EQ(screen.x, 400.0f - 32.0f);
    EXPECT_FLOAT_EQ(screen.y, 100.0f + 16.0f);
}

TEST(IsoProjection, ScreenToWorldInvertsWorldToScreen) {
    // Given
    const world::IsoProjection projection = scrolled_projection();
    const world::WorldPoint original{7.25f, -3.5f};

    // When
    const world::WorldPoint round_trip =
        projection.screen_to_world(projection.world_to_screen(original));

    // Then
    EXPECT_NEAR(round_trip.x, original.x, 1e-4f);
    EXPECT_NEAR(round_trip.y, original.y, 1e-4f);
}

TEST(IsoProjection, ZoomedTilesScaleTheProjection) {
    // Given
    world::IsoProjection projection;
    projection.tile_width = 128.0f;
    projection.tile_height = 64.0f;

    // When
    const world::ScreenPoint screen = projection.world_to_screen({2.0f, 1.0f});

    // Then
    EXPECT_FLOAT_EQ(screen.x, 64.0f);
    EXPECT_FLOAT_EQ(screen.y, 96.0f);
}

TEST(IsoProjection, TileAtPicksTheTileUnderItsCenter) {
    // Given
    const world::IsoProjection projection = scrolled_projection();
    const world::TileCoord tile{3, 2};

    // When
    const world::TileCoord picked = projection.tile_at(projection.tile_center(tile));

    // Then
    EXPECT_EQ(picked, tile);
}

TEST(IsoProjection, BoundingBoxCornersOfADiamondBelongToNeighbourTiles) {
    // Given: tile (0, 0) spans x 368..432, y 100..132 on screen, but only
    // its diamond is the tile; the box corners are other tiles.
    const world::IsoProjection projection = scrolled_projection();

    // When
    const world::TileCoord top_left = projection.tile_at({372.0f, 102.0f});
    const world::TileCoord top_right = projection.tile_at({428.0f, 102.0f});
    const world::TileCoord bottom_left = projection.tile_at({372.0f, 130.0f});
    const world::TileCoord bottom_right = projection.tile_at({428.0f, 130.0f});

    // Then
    EXPECT_EQ(top_left, (world::TileCoord{-1, 0}));
    EXPECT_EQ(top_right, (world::TileCoord{0, -1}));
    EXPECT_EQ(bottom_left, (world::TileCoord{0, 1}));
    EXPECT_EQ(bottom_right, (world::TileCoord{1, 0}));
}

TEST(IsoProjection, PickingAboveTheOriginGivesNegativeTiles) {
    // Given: a point just above the top corner of tile (0, 0).
    const world::IsoProjection projection = scrolled_projection();

    // When
    const world::TileCoord picked = projection.tile_at({400.0f, 92.0f});

    // Then: it floors to (-1, -1) instead of truncating to (0, 0).
    EXPECT_EQ(picked, (world::TileCoord{-1, -1}));
}

TEST(IsoProjection, TileCornersFormTheDiamondAroundTheCenter) {
    // Given
    const world::IsoProjection projection = scrolled_projection();

    // When
    const world::TileCorners corners = projection.tile_corners({1, 0});
    const world::ScreenPoint center = projection.tile_center({1, 0});

    // Then: tile (1, 0) is one step right-down from the origin tile.
    EXPECT_FLOAT_EQ(corners.top.x, 432.0f);
    EXPECT_FLOAT_EQ(corners.top.y, 116.0f);
    EXPECT_FLOAT_EQ(corners.right.x, 464.0f);
    EXPECT_FLOAT_EQ(corners.right.y, 132.0f);
    EXPECT_FLOAT_EQ(corners.bottom.x, 432.0f);
    EXPECT_FLOAT_EQ(corners.bottom.y, 148.0f);
    EXPECT_FLOAT_EQ(corners.left.x, 400.0f);
    EXPECT_FLOAT_EQ(corners.left.y, 132.0f);
    EXPECT_FLOAT_EQ(center.x, 432.0f);
    EXPECT_FLOAT_EQ(center.y, 132.0f);
}

TEST(IsoTile, TileContainingFloorsNegativePositions) {
    // Given
    const world::WorldPoint point{-0.25f, 2.75f};

    // When
    const world::TileCoord tile = world::tile_containing(point);

    // Then
    EXPECT_EQ(tile, (world::TileCoord{-1, 2}));
}

TEST(IsoDepth, PositionsLowerOnScreenAreDeeper) {
    // Given
    const world::IsoProjection projection;
    const world::WorldPoint back{1.0f, 1.0f};
    const world::WorldPoint front{2.5f, 0.5f};

    // When
    const float back_depth = world::depth_of(back);
    const float front_depth = world::depth_of(front);

    // Then: depth grows with the screen y of the position.
    EXPECT_LT(projection.world_to_screen(back).y, projection.world_to_screen(front).y);
    EXPECT_LT(back_depth, front_depth);
}

TEST(IsoDepth, SortingByDrawOrderPaintsBackTilesFirst) {
    // Given
    std::vector<world::TileCoord> tiles{{1, 1}, {0, 0}, {1, 0}, {0, 1}, {2, 2}};

    // When
    std::sort(tiles.begin(), tiles.end(), world::draws_before);

    // Then: back to front; within a screen row (0,1 is left of 1,0), left first.
    const std::vector<world::TileCoord> expected{{0, 0}, {0, 1}, {1, 0}, {1, 1}, {2, 2}};
    EXPECT_EQ(tiles, expected);
}

TEST(ScreenDirectionToWorld, ScreenUpGoesStraightUpOnScreen) {
    // Given
    const world::IsoProjection projection;

    // When
    const world::WorldPoint direction = world::screen_direction_to_world({0.0f, -1.0f});
    const world::ScreenPoint moved = projection.world_to_screen(direction);
    const world::ScreenPoint start = projection.world_to_screen({0.0f, 0.0f});

    // Then
    EXPECT_FLOAT_EQ(direction.x, -1.0f);
    EXPECT_FLOAT_EQ(direction.y, -1.0f);
    EXPECT_FLOAT_EQ(moved.x, start.x);
    EXPECT_LT(moved.y, start.y);
}

TEST(ScreenDirectionToWorld, ScreenRightGoesStraightRightOnScreen) {
    // Given
    const world::IsoProjection projection;

    // When
    const world::WorldPoint direction = world::screen_direction_to_world({1.0f, 0.0f});
    const world::ScreenPoint moved = projection.world_to_screen(direction);
    const world::ScreenPoint start = projection.world_to_screen({0.0f, 0.0f});

    // Then
    EXPECT_FLOAT_EQ(direction.x, 1.0f);
    EXPECT_FLOAT_EQ(direction.y, -1.0f);
    EXPECT_FLOAT_EQ(moved.y, start.y);
    EXPECT_GT(moved.x, start.x);
}
