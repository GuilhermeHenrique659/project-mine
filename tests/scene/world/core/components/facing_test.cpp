#include <gtest/gtest.h>

#include "scene/world/core/components/facing.hpp"

using world::Direction;
using world::facing_of;

TEST(Facing, ScreenAxesGiveTheFourMainDirections) {
    // Given: screen up is world (-1, -1), screen right is world (+1, -1)

    // When / Then
    EXPECT_EQ(facing_of({-1.0f, -1.0f}), Direction::North);
    EXPECT_EQ(facing_of({1.0f, -1.0f}), Direction::East);
    EXPECT_EQ(facing_of({1.0f, 1.0f}), Direction::South);
    EXPECT_EQ(facing_of({-1.0f, 1.0f}), Direction::West);
}

TEST(Facing, WorldAxesGiveTheDiagonals) {
    // Given: world +x is screen right-down, world +y screen left-down

    // When / Then
    EXPECT_EQ(facing_of({1.0f, 0.0f}), Direction::SouthEast);
    EXPECT_EQ(facing_of({0.0f, 1.0f}), Direction::SouthWest);
    EXPECT_EQ(facing_of({-1.0f, 0.0f}), Direction::NorthWest);
    EXPECT_EQ(facing_of({0.0f, -1.0f}), Direction::NorthEast);
}

TEST(Facing, ADirectionSnapsToTheClosestOfTheEight) {
    // Given: mostly screen right, a little up (world (1, -1) plus a bit of (-1, -1))

    // When / Then
    EXPECT_EQ(facing_of({0.9f, -1.1f}), Direction::East);
}
