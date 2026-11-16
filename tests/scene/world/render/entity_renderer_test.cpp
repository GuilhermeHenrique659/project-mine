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
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/worker.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

// Every cube is 6 triangles (top, left, right faces). The second triangle of the left face
// runs top-left, front-bottom, left-bottom: the ground point is (front.x, left.y).
constexpr std::size_t TrianglesPerCube = 6;

ui::Point ground_of(const FakeCanvas& canvas, std::size_t cube) {
    const FilledTriangle& left = canvas.triangles[cube * TrianglesPerCube + 3];
    return {left.b.x, left.c.y};
}

// Width of a cube on screen: from the left face's outer edge to the right face's.
float width_of(const FakeCanvas& canvas, std::size_t cube) {
    const FilledTriangle& left = canvas.triangles[cube * TrianglesPerCube + 3];
    const FilledTriangle& right = canvas.triangles[cube * TrianglesPerCube + 5];
    return right.c.x - left.c.x;
}

// The highest point of a cube (its back top corner).
float top_of(const FakeCanvas& canvas, std::size_t cube) {
    return canvas.triangles[cube * TrianglesPerCube].a.y;
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
    ASSERT_EQ(canvas.triangles.size(), TrianglesPerCube);
    const ui::Point ground = ground_of(canvas, 0);
    EXPECT_FLOAT_EQ(ground.x, 640.0f + 64.0f);
    EXPECT_FLOAT_EQ(ground.y, 360.0f + 32.0f);
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts.front().text, "Player (Moving)");
    EXPECT_LT(canvas.texts.front().position.y, top_of(canvas, 0));
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
    EXPECT_EQ(canvas.triangles.size(), TrianglesPerCube);
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
    ASSERT_EQ(canvas.triangles.size(), TrianglesPerCube);
    const ui::Point ground = ground_of(canvas, 0);
    EXPECT_FLOAT_EQ(ground.x, 640.0f);
    EXPECT_FLOAT_EQ(ground.y, 360.0f);
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
    ASSERT_EQ(canvas.triangles.size(), TrianglesPerCube);
    const ui::Point ground = ground_of(canvas, 0);
    EXPECT_FLOAT_EQ(ground.x, 640.0f - 64.0f);
    EXPECT_FLOAT_EQ(ground.y, 360.0f - 32.0f);
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

    // Then: the character (cube 0) is drawn before the building (cube 1)
    ASSERT_EQ(canvas.triangles.size(), 2 * TrianglesPerCube);
    EXPECT_GT(width_of(canvas, 1), width_of(canvas, 0));
    ASSERT_EQ(canvas.texts.size(), 2u);
    EXPECT_EQ(canvas.texts[1].text, "Mine (25%)");
}

TEST(EntityRenderer, ABuildingIsDrawnAsWideAsItsFootprint) {
    // Given: a finished 5x5 building centred on its footprint, with 64-pixel tiles
    core::Registry registry;
    const core::EntityId hq = registry.create("Headquarters");
    registry.add(hq, world::Building{world::BuildingType::Headquarters, {0, 0}, {5, 5}});
    registry.add(hq, world::Position{{2.5f, 2.5f}});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    ASSERT_EQ(canvas.triangles.size(), TrianglesPerCube);
    EXPECT_FLOAT_EQ(width_of(canvas, 0), 5.0f * 64.0f);
}

TEST(EntityRenderer, AWorkerAtWorkInsideItsBuildingIsNotDrawn) {
    // Given: a finished mine on (4, 0) with one worker at it and one still walking there
    core::Registry registry;
    const core::EntityId mine = registry.create("Mine");
    registry.add(mine, world::Building{world::BuildingType::Mine, {4, 0}});
    registry.add(mine, world::Position{{4.5f, 0.5f}});
    const core::EntityId inside = world::spawn_worker(registry, "Worker", {4.5f, 0.5f});
    const core::EntityId walking = world::spawn_worker(registry, "Worker", {0.5f, 0.5f});
    *registry.get<world::Worker>(inside) = {world::WorkerState::Working, mine};
    *registry.get<world::Worker>(walking) = {world::WorkerState::Working, mine};
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: the mine and the walking worker only
    EXPECT_EQ(canvas.triangles.size(), 2 * TrianglesPerCube);
    EXPECT_EQ(canvas.texts.size(), 2u);
}
