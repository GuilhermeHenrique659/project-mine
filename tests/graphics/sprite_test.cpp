#include <gtest/gtest.h>

#include <memory>

#include "graphics/sprite.hpp"
#include "graphics/texture_manager.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_texture_loader.hpp"

namespace {

class DrawSprite : public testing::Test {
protected:
    void SetUp() override {
        graphics::TextureManager::instance().reset();
        graphics::TextureManager::instance().set_loader(std::make_unique<FakeTextureLoader>());
        graphics::TextureManager::instance().register_texture("hero", "hero.png");
    }
    void TearDown() override { graphics::TextureManager::instance().reset(); }

    FakeCanvas canvas;
};

} // namespace

TEST_F(DrawSprite, DrawsOnlyTheSpritesRegionWithTheRequestedOptions) {
    // Given
    const graphics::Sprite sprite{"hero", ui::Rect{16.0f, 0.0f, 16.0f, 32.0f}};
    ui::TextureDraw options;
    options.flip_x = true;
    options.tint = {255, 0, 0, 255};

    // When
    const bool drawn =
        graphics::draw_sprite(canvas, sprite, {10.0f, 20.0f, 32.0f, 64.0f}, options);

    // Then
    EXPECT_TRUE(drawn);
    ASSERT_EQ(canvas.textures.size(), 1u);
    const DrawnTexture& call = canvas.textures.front();
    EXPECT_EQ(call.texture.id, graphics::TextureManager::instance().get("hero")->id);
    EXPECT_FLOAT_EQ(call.dest.width, 32.0f);
    ASSERT_TRUE(call.options.source.has_value());
    EXPECT_FLOAT_EQ(call.options.source->x, 16.0f);
    EXPECT_TRUE(call.options.flip_x);
    EXPECT_EQ(call.options.tint, (ui::Color{255, 0, 0, 255}));
}

TEST_F(DrawSprite, SimpleSpriteDrawsTheWholeTexture) {
    // Given
    const graphics::Sprite sprite{"hero", std::nullopt};

    // When
    graphics::draw_sprite(canvas, sprite, {0.0f, 0.0f, 64.0f, 64.0f});

    // Then
    ASSERT_EQ(canvas.textures.size(), 1u);
    EXPECT_FALSE(canvas.textures.front().options.source.has_value());
}

TEST_F(DrawSprite, MissingTextureDrawsTheWholeFallbackInsteadOfTheSpritesRegion) {
    // Given
    const graphics::Sprite sprite{"unknown", ui::Rect{16.0f, 0.0f, 16.0f, 32.0f}};

    // When
    const bool drawn = graphics::draw_sprite(canvas, sprite, {0.0f, 0.0f, 64.0f, 64.0f});

    // Then: the region belongs to the missing texture, so it makes no sense on the fallback
    EXPECT_TRUE(drawn);
    ASSERT_EQ(canvas.textures.size(), 1u);
    EXPECT_EQ(canvas.textures.front().texture.id, FakeTextureLoader::PlaceholderId);
    EXPECT_FALSE(canvas.textures.front().options.source.has_value());
}

TEST_F(DrawSprite, DrawsNothingAndReportsItWhenThereIsNoFallbackEither) {
    // Given
    graphics::TextureManager::instance().reset(); // no loader
    const graphics::Sprite sprite{"unknown", std::nullopt};

    // When
    const bool drawn = graphics::draw_sprite(canvas, sprite, {0.0f, 0.0f, 64.0f, 64.0f});

    // Then
    EXPECT_FALSE(drawn);
    EXPECT_TRUE(canvas.textures.empty());
}
