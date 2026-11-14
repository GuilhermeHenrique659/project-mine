#include <gtest/gtest.h>

#include <algorithm>

#include "scene/world/core/isometric.hpp"
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
    const scene::world::TerrainRenderer renderer(terrain);
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
    const scene::world::TerrainRenderer renderer(terrain);
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
