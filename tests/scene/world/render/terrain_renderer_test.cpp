#include <gtest/gtest.h>

#include <algorithm>

#include "scene/world/core/isometric.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/render/terrain_renderer.hpp"
#include "support/fake_canvas.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

bool same_color(ui::Color a, ui::Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// The colour of the triangles covering the tile whose top corner is `top`, or none.
bool tile_is_drawn_with(const FakeCanvas& canvas, world::ScreenPoint top, ui::Color color) {
    return std::any_of(canvas.triangles.begin(), canvas.triangles.end(),
                       [&](const FilledTriangle& triangle) {
                           return triangle.a.x == top.x && triangle.a.y == top.y &&
                                  same_color(triangle.color, color);
                       });
}

} // namespace

TEST(TerrainRenderer, GrassTilesAreGreenAndWaterTilesAreBlue) {
    // Given
    const world::Terrain terrain(2024);
    const world::Camera camera;
    const scene::world::TerrainRenderer renderer(terrain, camera);
    FakeCanvas canvas;
    world::IsoProjection projection;
    projection.origin = {Screen.width / 2.0f, Screen.height / 2.0f};

    // When
    renderer.draw(canvas, Screen);

    // Then: tiles around the centre are on screen and have the colour of their type
    int grass = 0;
    int water = 0;
    for (int y = -6; y <= 6; ++y) {
        for (int x = -6; x <= 6; ++x) {
            const world::TileType type = terrain.tile_at({x, y});
            const world::ScreenPoint top = projection.tile_corners({x, y}).top;
            if (type == world::TileType::Grass) {
                EXPECT_TRUE(tile_is_drawn_with(canvas, top, scene::world::GrassColor)) << x << "," << y;
                ++grass;
            } else {
                EXPECT_TRUE(tile_is_drawn_with(canvas, top, scene::world::WaterColor)) << x << "," << y;
                ++water;
            }
        }
    }
    EXPECT_GT(grass, 0);
}

TEST(TerrainRenderer, EveryPartOfTheScreenIsCovered) {
    // Given
    const world::Terrain terrain(5);
    const world::Camera camera;
    const scene::world::TerrainRenderer renderer(terrain, camera);
    FakeCanvas canvas;
    world::IsoProjection projection;
    projection.origin = {Screen.width / 2.0f, Screen.height / 2.0f};

    // When
    renderer.draw(canvas, Screen);

    // Then: the tiles under the four corners of the screen are drawn
    for (const world::ScreenPoint corner :
         {world::ScreenPoint{0.0f, 0.0f}, {Screen.width, 0.0f}, {0.0f, Screen.height},
          {Screen.width, Screen.height}}) {
        const world::ScreenPoint top = projection.tile_corners(projection.tile_at(corner)).top;
        const bool drawn = tile_is_drawn_with(canvas, top, scene::world::GrassColor) ||
                           tile_is_drawn_with(canvas, top, scene::world::WaterColor);
        EXPECT_TRUE(drawn) << corner.x << "," << corner.y;
    }
}

TEST(TerrainRenderer, TilesAroundTheCameraAreDrawnWhenItIsFarFromTheOrigin) {
    // Given
    const world::Terrain terrain(2024);
    world::Camera camera;
    camera.center = {500.0f, -300.0f};
    const scene::world::TerrainRenderer renderer(terrain, camera);
    FakeCanvas canvas;
    const world::IsoProjection projection = camera.projection(Screen.width, Screen.height);

    // When
    renderer.draw(canvas, Screen);

    // Then: the tile under the middle of the screen is the one the camera looks at
    const world::TileCoord middle = projection.tile_at({Screen.width / 2.0f, Screen.height / 2.0f});
    ASSERT_EQ(middle, (world::TileCoord{500, -300}));
    const ui::Color color = terrain.tile_at(middle) == world::TileType::Water
                                ? scene::world::WaterColor
                                : scene::world::GrassColor;
    EXPECT_TRUE(tile_is_drawn_with(canvas, projection.tile_corners(middle).top, color));
}
