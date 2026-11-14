#include <gtest/gtest.h>

#include <memory>

#include "core/registry.hpp"
#include "graphics/texture_manager.hpp"
#include "scene/world/render/entity_renderer.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_texture_loader.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/entities/character.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(EntityRenderer, CharacterIsDrawnAsACubeWithItsLabelAndStateAroundTheScreenCentre) {
    // Given
    core::Registry registry;
    const core::EntityId id = world::spawn_character(registry, "Player", {2.0f, 0.0f});
    registry.get<world::Character>(id)->state = world::CharacterState::Moving;
    const scene::world::EntityRenderer renderer(registry);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: world (2, 0) with 64x32 tiles is (64, 32) pixels from the screen centre
    ASSERT_EQ(canvas.fills.size(), 2u); // top face, then front face
    const ui::Point marker_center = center_of(canvas.fills.back().rect);
    EXPECT_FLOAT_EQ(marker_center.x, 640.0f + 64.0f);
    EXPECT_FLOAT_EQ(marker_center.y, 360.0f + 32.0f);
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts.front().text, "Player (Moving)");
    EXPECT_LT(canvas.texts.front().position.y, canvas.fills.front().rect.y);
}

TEST(EntityRenderer, EntityFurtherDownTheScreenIsDrawnAfterOneBehindIt) {
    // Given: the near entity is created first, so creation order cannot explain the result
    core::Registry registry;
    const core::EntityId near_id = registry.create("Near");
    registry.add(near_id, world::Position{{3.0f, 3.0f}});
    const core::EntityId far_id = registry.create("Far");
    registry.add(far_id, world::Position{{0.0f, 0.0f}});
    const scene::world::EntityRenderer renderer(registry);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    ASSERT_EQ(canvas.texts.size(), 2u);
    EXPECT_EQ(canvas.texts[0].text, "Far");
    EXPECT_EQ(canvas.texts[1].text, "Near");
}

namespace {

class EntityRendererTexture : public testing::Test {
protected:
    void SetUp() override {
        graphics::TextureManager::instance().reset();
        graphics::TextureManager::instance().set_loader(std::make_unique<FakeTextureLoader>());
    }
    void TearDown() override { graphics::TextureManager::instance().reset(); }
};

} // namespace

TEST_F(EntityRendererTexture, CharacterIsDrawnWithItsTextureInsteadOfTheCubeWhenOneIsRegistered) {
    // Given
    graphics::TextureManager::instance().register_texture(
        scene::world::CharacterTexture, "character.png");
    core::Registry registry;
    world::spawn_character(registry, "Player", {0.0f, 0.0f});
    const scene::world::EntityRenderer renderer(registry);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: the texture sits on the entity's position and the cube is not drawn
    ASSERT_EQ(canvas.textures.size(), 1u);
    EXPECT_EQ(canvas.textures.front().texture.id, 1u); // the character.png the loader made
    const ui::Point texture_center = center_of(canvas.textures.front().dest);
    EXPECT_FLOAT_EQ(texture_center.x, 640.0f);
    EXPECT_FLOAT_EQ(texture_center.y, 360.0f);
    EXPECT_TRUE(canvas.fills.empty());
    EXPECT_EQ(canvas.texts.size(), 1u); // the label is still shown
}

TEST_F(EntityRendererTexture, CharacterWithoutARegisteredTextureIsDrawnWithTheFallbackTexture) {
    // Given
    core::Registry registry;
    world::spawn_character(registry, "Player", {0.0f, 0.0f});
    const scene::world::EntityRenderer renderer(registry);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    ASSERT_EQ(canvas.textures.size(), 1u);
    EXPECT_EQ(canvas.textures.front().texture.id, FakeTextureLoader::PlaceholderId);
    EXPECT_TRUE(canvas.fills.empty());
}

TEST_F(EntityRendererTexture, CharacterIsDrawnAsACubeWhenThereIsNoTextureNotEvenTheFallback) {
    // Given
    graphics::TextureManager::instance().reset(); // no loader
    core::Registry registry;
    world::spawn_character(registry, "Player", {0.0f, 0.0f});
    const scene::world::EntityRenderer renderer(registry);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    EXPECT_TRUE(canvas.textures.empty());
    EXPECT_EQ(canvas.fills.size(), 2u);
}
