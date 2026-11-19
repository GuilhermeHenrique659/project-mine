#include <gtest/gtest.h>

#include <algorithm>

#include "scene/world/core/isometric.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/render/terrain_renderer.hpp"
#include "graphics/texture_manager.hpp"
#include "graphics/game_sprites.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_texture_loader.hpp"

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

TEST(TerrainRenderer, EveryTileIsDrawnWithTheColourOfItsType) {
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
    for (int y = -6; y <= 6; ++y) {
        for (int x = -6; x <= 6; ++x) {
            const world::ScreenPoint top = projection.tile_corners({x, y}).top;
            const ui::Color color = scene::world::tile_color(terrain.tile_at({x, y}));
            EXPECT_TRUE(tile_is_drawn_with(canvas, top, color)) << x << "," << y;
        }
    }
}

TEST(TerrainRenderer, OreTilesAreDrawnWithTheirOreColour) {
    // Given: a camera looking at a starting ore node
    const world::Terrain terrain(2024);
    world::Camera camera;
    world::TileCoord ore_tile{0, 0};
    for (int y = -40; y <= 40; ++y) {
        for (int x = -40; x <= 40; ++x) {
            if (world::is_ore(terrain.tile_at({x, y}))) {
                ore_tile = {x, y};
            }
        }
    }
    ASSERT_TRUE(world::is_ore(terrain.tile_at(ore_tile)));
    camera.center = {static_cast<float>(ore_tile.x), static_cast<float>(ore_tile.y)};
    const scene::world::TerrainRenderer renderer(terrain, camera);
    FakeCanvas canvas;
    const world::IsoProjection projection = camera.projection(Screen.width, Screen.height);

    // When
    renderer.draw(canvas, Screen);

    // Then: the ore colour differs from grass and water
    const ui::Color color = scene::world::tile_color(terrain.tile_at(ore_tile));
    EXPECT_TRUE(tile_is_drawn_with(canvas, projection.tile_corners(ore_tile).top, color));
    EXPECT_FALSE(same_color(color, scene::world::GrassColor));
    EXPECT_FALSE(same_color(color, scene::world::WaterColor));
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
        const bool drawn = tile_is_drawn_with(canvas, top, scene::world::tile_color(terrain.tile_at(projection.tile_at(corner))));
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
    const ui::Color color = scene::world::tile_color(terrain.tile_at(middle));
    EXPECT_TRUE(tile_is_drawn_with(canvas, projection.tile_corners(middle).top, color));
}

namespace {

// Every tile type with a one-frame 64x32 sprite whose sheet loads (water animated).
graphics::GameSprites every_tile_sprite() {
    auto& textures = graphics::TextureManager::instance();
    graphics::SpriteLibrary library;
    for (const world::TileType type : {world::TileType::Grass, world::TileType::Water,
                                       world::TileType::IronOre, world::TileType::CopperOre,
                                       world::TileType::Coal}) {
        const std::string name = graphics::tile_sprite_name(type);
        library.add(name,
                    {"tile.png", 64, 32, 8, 4, {}, {graphics::SpriteClip{"idle", 0, 8, 8.0f, true}}},
                    textures.register_texture(name, name + ".png"));
    }
    return graphics::GameSprites(std::move(library));
}

} // namespace

TEST(TerrainRenderer, TilesWithATextureAreDrawnWithTheirSpriteOverTheDiamond) {
    // Given: every terrain texture loads, and 1.3 s of animation have passed
    auto& textures = graphics::TextureManager::instance();
    textures.reset();
    textures.set_loader(std::make_unique<FakeTextureLoader>());
    const graphics::GameSprites sprites = every_tile_sprite();
    const world::Terrain terrain(2024);
    const world::Camera camera;
    const scene::world::TerrainRenderer renderer(terrain, camera, sprites, [] { return 1.3f; });
    FakeCanvas canvas;
    const world::IsoProjection projection = camera.projection(Screen.width, Screen.height);

    // When
    renderer.draw(canvas, Screen);

    // Then: the centre tile's current frame fills the box around its diamond
    const world::TileCorners diamond = projection.tile_corners({0, 0});
    const auto expected = sprites.tile(terrain.tile_at({0, 0}), {0, 0}, 1.3f);
    ASSERT_TRUE(expected);
    const bool found = std::any_of(
        canvas.textures.begin(), canvas.textures.end(), [&](const DrawnTexture& drawn) {
            return drawn.dest.x == diamond.left.x && drawn.dest.y == diamond.top.y &&
                   drawn.dest.width == diamond.right.x - diamond.left.x &&
                   drawn.dest.height == diamond.bottom.y - diamond.top.y &&
                   drawn.options.source && drawn.options.source->x == expected->source.x &&
                   drawn.options.source->y == expected->source.y;
        });
    textures.reset();
    EXPECT_TRUE(found);
}

TEST(TerrainRenderer, ATileDrawnWithItsSpriteSkipsItsPlainColour) {
    // Given: every terrain texture loads
    auto& textures = graphics::TextureManager::instance();
    textures.reset();
    textures.set_loader(std::make_unique<FakeTextureLoader>());
    const graphics::GameSprites sprites = every_tile_sprite();
    const world::Terrain terrain(2024);
    const world::Camera camera;
    const scene::world::TerrainRenderer renderer(terrain, camera, sprites);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: no diamond painted underneath
    textures.reset();
    EXPECT_FALSE(canvas.textures.empty());
    EXPECT_TRUE(canvas.triangles.empty());
}
