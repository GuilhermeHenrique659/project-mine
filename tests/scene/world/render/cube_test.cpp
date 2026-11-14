#include <gtest/gtest.h>

#include "scene/world/render/cube.hpp"
#include "support/fake_canvas.hpp"

TEST(Cube, FrontFaceIsASquareOfItsSizeCentredOnThePositionWithTheTopFaceAbove) {
    // Given
    const scene::world::Cube cube(40.0f);
    FakeCanvas canvas;

    // When
    cube.draw(canvas, {100.0f, 200.0f});

    // Then: top face first, then the front face
    ASSERT_EQ(canvas.fills.size(), 2u);
    const ui::Rect top = canvas.fills[0].rect;
    const ui::Rect front = canvas.fills[1].rect;
    EXPECT_FLOAT_EQ(front.width, 40.0f);
    EXPECT_FLOAT_EQ(front.height, 40.0f);
    EXPECT_FLOAT_EQ(front.x + front.width / 2.0f, 100.0f);
    EXPECT_FLOAT_EQ(front.y + front.height / 2.0f, 200.0f);
    EXPECT_FLOAT_EQ(top.width, 40.0f);
    EXPECT_FLOAT_EQ(top.y + top.height, front.y); // sits right on top of the front face
}

TEST(Cube, BiggerCubeHasProportionallyBiggerFacesAndBounds) {
    // Given
    const scene::world::Cube small(20.0f);
    const scene::world::Cube big(60.0f);

    // When
    const ui::Rect small_bounds = small.bounds({0.0f, 0.0f});
    const ui::Rect big_bounds = big.bounds({0.0f, 0.0f});

    // Then
    EXPECT_FLOAT_EQ(big_bounds.width, 3.0f * small_bounds.width);
    EXPECT_FLOAT_EQ(big_bounds.height, 3.0f * small_bounds.height);
}

TEST(Cube, BoundsCoverBothFaces) {
    // Given
    const scene::world::Cube cube(30.0f);
    FakeCanvas canvas;

    // When
    cube.draw(canvas, {50.0f, 50.0f});

    // Then
    const ui::Rect bounds = cube.bounds({50.0f, 50.0f});
    EXPECT_FLOAT_EQ(bounds.y, canvas.fills[0].rect.y);
    EXPECT_FLOAT_EQ(bounds.y + bounds.height, canvas.fills[1].rect.y + canvas.fills[1].rect.height);
}
