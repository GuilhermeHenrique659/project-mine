#include <gtest/gtest.h>

#include <set>
#include <string>

#include "graphics/terrain_sprites.hpp"

using graphics::tile_sprite;
using world::TileType;

TEST(TerrainSprites, AGrassTileAlwaysKeepsTheSameVariant) {
    // Given
    const world::TileCoord tile{7, -3};

    // When
    const graphics::Sprite first = tile_sprite(TileType::Grass, tile, 0.0f);
    const graphics::Sprite later = tile_sprite(TileType::Grass, tile, 5.3f);

    // Then
    EXPECT_EQ(first.texture.rfind("grass_", 0), 0u);
    EXPECT_EQ(first.texture, later.texture);
}

TEST(TerrainSprites, AGrassFieldUsesEveryVariant) {
    // Given
    std::set<std::string> used;

    // When
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            used.insert(tile_sprite(TileType::Grass, {x, y}, 0.0f).texture);
        }
    }

    // Then
    EXPECT_EQ(used.size(), static_cast<std::size_t>(graphics::GrassVariants));
}

TEST(TerrainSprites, EachOreHasItsOwnSprite) {
    // When / Then
    EXPECT_EQ(tile_sprite(TileType::IronOre, {0, 0}, 0.0f).texture, "iron_ore");
    EXPECT_EQ(tile_sprite(TileType::CopperOre, {0, 0}, 0.0f).texture, "copper_ore");
    EXPECT_EQ(tile_sprite(TileType::Coal, {0, 0}, 0.0f).texture, "coal");
}

TEST(TerrainSprites, WaterPlaysItsFramesInALoop) {
    // Given: frame n starts at n / 8 seconds
    const float frame = 1.0f / graphics::WaterFramesPerSecond;

    // When
    const graphics::Sprite start = tile_sprite(TileType::Water, {2, 2}, 0.0f);
    const graphics::Sprite third = tile_sprite(TileType::Water, {2, 2}, 2.5f * frame);
    const graphics::Sprite looped = tile_sprite(TileType::Water, {2, 2}, 9.5f * frame);

    // Then: frames are 64x32 cells side by side in the sheet
    EXPECT_EQ(start.texture, "water");
    ASSERT_TRUE(start.source && third.source && looped.source);
    EXPECT_FLOAT_EQ(start.source->x, 0.0f);
    EXPECT_FLOAT_EQ(third.source->x, 128.0f);
    EXPECT_FLOAT_EQ(third.source->width, 64.0f);
    EXPECT_FLOAT_EQ(third.source->height, 32.0f);
    EXPECT_FLOAT_EQ(looped.source->x, 64.0f);
}
