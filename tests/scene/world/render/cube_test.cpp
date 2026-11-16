#include <gtest/gtest.h>

#include <algorithm>

#include "scene/world/render/cube.hpp"
#include "support/fake_canvas.hpp"

namespace {

constexpr world::ScreenPoint Ground{100.0f, 200.0f};

} // namespace

TEST(Cube, TheTopFaceIsATwoToOneDiamondAsWideAsTheCube) {
    // Given
    const scene::world::Cube cube(40.0f);

    // When
    const scene::world::CubeFaces faces = cube.faces(Ground);

    // Then: corners top, right, bottom, left, like a tile
    const auto& top = faces.top;
    EXPECT_FLOAT_EQ(top[1].x - top[3].x, 40.0f);
    EXPECT_FLOAT_EQ(top[2].y - top[0].y, 20.0f);
    EXPECT_FLOAT_EQ(top[0].x, Ground.x);
    EXPECT_FLOAT_EQ(top[2].x, Ground.x);
    EXPECT_FLOAT_EQ(top[1].y, top[3].y);
}

TEST(Cube, AFootprintCubeCoversItsTilesAlongBothWorldAxes) {
    // Given: 2 tiles along world x, 1 along world y, with 40-pixel-wide tiles
    const scene::world::Cube cube = scene::world::Cube::footprint(40.0f, {2, 1});

    // When
    const scene::world::CubeFaces faces = cube.faces(Ground);

    // Then: world +x runs down-right (20, 10 per tile), world +y down-left (-20, 10)
    const auto& left = faces.left;  // top-outer, top-front, bottom-front, bottom-outer
    const auto& right = faces.right;
    EXPECT_FLOAT_EQ(left[2].x, Ground.x + 10.0f);  // front corner: +x/2 + y/2
    EXPECT_FLOAT_EQ(left[2].y, Ground.y + 15.0f);
    EXPECT_FLOAT_EQ(left[3].x, Ground.x - 30.0f);  // left corner: -x/2 + y/2
    EXPECT_FLOAT_EQ(right[3].x, Ground.x + 30.0f); // right corner: +x/2 - y/2
    const ui::Rect bounds = cube.bounds(Ground);
    EXPECT_FLOAT_EQ(bounds.width, 60.0f);
    EXPECT_FLOAT_EQ(bounds.y + bounds.height, Ground.y + 15.0f);
}

TEST(Cube, AFootprintBoxIsOneTileTallWhateverItsFootprint) {
    // Given
    const scene::world::Cube big = scene::world::Cube::footprint(40.0f, {5, 5});
    const scene::world::Cube one_tile(40.0f);

    // When
    const float big_height = big.height();
    const float tile_height = one_tile.height();

    // Then: a 5x5x1 slab, not a 5x5x5 cube
    EXPECT_FLOAT_EQ(big_height, tile_height);
    const scene::world::CubeFaces faces = big.faces(Ground);
    EXPECT_FLOAT_EQ(faces.left[3].y - faces.left[0].y, tile_height);
}

TEST(Cube, ItsBaseIsTheDiamondCentredOnTheGroundPoint) {
    // Given
    const scene::world::Cube cube(40.0f);

    // When
    const scene::world::CubeFaces faces = cube.faces(Ground);

    // Then: the side faces end on the ground diamond's left, bottom and right corners
    EXPECT_FLOAT_EQ(faces.left[3].x, Ground.x - 20.0f);
    EXPECT_FLOAT_EQ(faces.left[3].y, Ground.y);
    EXPECT_FLOAT_EQ(faces.left[2].x, Ground.x);
    EXPECT_FLOAT_EQ(faces.left[2].y, Ground.y + 10.0f);
    EXPECT_FLOAT_EQ(faces.right[3].x, Ground.x + 20.0f);
    EXPECT_FLOAT_EQ(faces.right[3].y, Ground.y);
}

TEST(Cube, TheSideFacesRiseStraightUpToTheTopFace) {
    // Given
    const scene::world::Cube cube(40.0f);

    // When
    const scene::world::CubeFaces faces = cube.faces(Ground);

    // Then: every vertical edge has the cube's height
    const float height = cube.height();
    EXPECT_GT(height, 0.0f);
    EXPECT_FLOAT_EQ(faces.left[3].y - faces.left[0].y, height);  // left edge
    EXPECT_FLOAT_EQ(faces.left[2].y - faces.left[1].y, height);  // front edge
    EXPECT_FLOAT_EQ(faces.right[3].y - faces.right[0].y, height); // right edge
    EXPECT_FLOAT_EQ(faces.left[0].x, faces.left[3].x);
    EXPECT_FLOAT_EQ(faces.left[1].x, faces.top[2].x);
}

TEST(Cube, DrawsThreeFacesLitFromAboveWithTheRightFaceDarkest) {
    // Given
    const scene::world::Cube cube(40.0f, {200, 160, 50, 255}, {240, 210, 110, 255});
    FakeCanvas canvas;

    // When
    cube.draw(canvas, Ground);

    // Then: two triangles per face, top then left then right
    ASSERT_EQ(canvas.triangles.size(), 6u);
    EXPECT_TRUE(canvas.fills.empty());
    const ui::Color top = canvas.triangles[0].color;
    const ui::Color left = canvas.triangles[2].color;
    const ui::Color right = canvas.triangles[4].color;
    EXPECT_GT(top.r, left.r);
    EXPECT_GT(left.r, right.r);
    EXPECT_EQ(left.a, right.a);
}

TEST(Cube, BoundsCoverEveryFace) {
    // Given
    const scene::world::Cube cube(30.0f);
    FakeCanvas canvas;

    // When
    cube.draw(canvas, Ground);

    // Then
    const ui::Rect bounds = cube.bounds(Ground);
    float min_y = 1e9f;
    float max_y = -1e9f;
    float min_x = 1e9f;
    float max_x = -1e9f;
    for (const auto& triangle : canvas.triangles) {
        for (const ui::Point& p : {triangle.a, triangle.b, triangle.c}) {
            min_x = std::min(min_x, p.x);
            max_x = std::max(max_x, p.x);
            min_y = std::min(min_y, p.y);
            max_y = std::max(max_y, p.y);
        }
    }
    EXPECT_FLOAT_EQ(bounds.x, min_x);
    EXPECT_FLOAT_EQ(bounds.y, min_y);
    EXPECT_FLOAT_EQ(bounds.x + bounds.width, max_x);
    EXPECT_FLOAT_EQ(bounds.y + bounds.height, max_y);
}

TEST(Cube, BiggerCubeHasProportionallyBiggerBounds) {
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
