#include <gtest/gtest.h>

#include <memory>

#include "core/registry.hpp"
#include "graphics/texture_manager.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/render/entity_renderer.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_texture_loader.hpp"
#include "scene/world/core/components/building.hpp"
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
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
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
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
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

TEST_F(EntityRendererTexture, CharacterIsDrawnAsACubeEvenWhenTheTextureLoaderHasAFallback) {
    // Given: the character has no texture, so the loader would give the checkerboard fallback
    core::Registry registry;
    world::spawn_character(registry, "Player", {0.0f, 0.0f});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    EXPECT_TRUE(canvas.textures.empty());
    EXPECT_EQ(canvas.fills.size(), 2u);
    EXPECT_EQ(canvas.texts.size(), 1u); // the label is still shown
}

TEST(EntityRenderer, EntityUnderTheCameraIsDrawnAtTheScreenCentreWhateverItsWorldPosition) {
    // Given
    core::Registry registry;
    world::spawn_character(registry, "Player", {40.0f, -12.0f});
    world::Camera camera;
    camera.center = {40.0f, -12.0f};
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    ASSERT_EQ(canvas.fills.size(), 2u);
    const ui::Point marker_center = center_of(canvas.fills.back().rect);
    EXPECT_FLOAT_EQ(marker_center.x, 640.0f);
    EXPECT_FLOAT_EQ(marker_center.y, 360.0f);
}

TEST(EntityRenderer, EntitiesShiftOnScreenOppositeToTheCameraMovement) {
    // Given: an entity at the origin while the camera is two tiles along world +x
    core::Registry registry;
    world::spawn_character(registry, "Rock", {0.0f, 0.0f});
    world::Camera camera;
    camera.center = {2.0f, 0.0f};
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: it is (64, 32) pixels up-left of the screen centre
    ASSERT_EQ(canvas.fills.size(), 2u);
    const ui::Point marker_center = center_of(canvas.fills.back().rect);
    EXPECT_FLOAT_EQ(marker_center.x, 640.0f - 64.0f);
    EXPECT_FLOAT_EQ(marker_center.y, 360.0f - 32.0f);
}

TEST(EntityRenderer, ABuildingIsABiggerCubeAndASiteShowsItsProgress) {
    // Given
    core::Registry registry;
    const core::EntityId character = world::spawn_character(registry, "Player", {0.0f, 0.0f});
    const core::EntityId site = registry.create("Mine");
    registry.add(site, world::Position{{4.0f, 4.0f}});
    registry.add(site, world::Building{});
    registry.add(site, world::ConstructionSite{1.0f, 4.0f});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;
    (void)character;

    // When
    renderer.draw(canvas, Screen);

    // Then: the character (front at fills[1]) is drawn before the building (fills[3])
    ASSERT_EQ(canvas.fills.size(), 4u);
    EXPECT_GT(canvas.fills[3].rect.width, canvas.fills[1].rect.width);
    ASSERT_EQ(canvas.texts.size(), 2u);
    EXPECT_EQ(canvas.texts[1].text, "Mine (25%)");
}
