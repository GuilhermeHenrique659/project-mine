#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "core/registry.hpp"
#include "graphics/game_sprites.hpp"
#include "graphics/texture_manager.hpp"
#include "scene/world/core/components/facing.hpp"
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

    // A library holding only `name`, whose sheet loads.
    static graphics::GameSprites sprites_with(const std::string& name,
                                              graphics::SpriteDefinition definition) {
        graphics::SpriteLibrary library;
        library.add(name, std::move(definition),
                    graphics::TextureManager::instance().register_texture(name, name + ".png"));
        return graphics::GameSprites(std::move(library));
    }

    // 8 frames of 336x352, anchored at (168, 264), at 4 fps.
    static graphics::SpriteDefinition headquarters() {
        return {"hq.png", 336, 352, 8, 1, {168.0f, 264.0f},
                {graphics::SpriteClip{"idle", 0, 8, 4.0f, true}}};
    }

    // 8 facings of standing + 6 walk frames at 10 fps, 32x48 anchored at (16, 44).
    static graphics::SpriteDefinition character() {
        return {"character.png", 32, 48, 7, 8, {16.0f, 44.0f},
                {graphics::SpriteClip{"idle", 0, 1, 0.0f, true},
                 graphics::SpriteClip{"walk", 1, 6, 10.0f, true}}};
    }
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

TEST(EntityRenderer, AWorkerIsLabelledWithTheWorkItDoes) {
    // Given: a worker walking to its mine and an idle one
    core::Registry registry;
    const core::EntityId mine = registry.create("Mine");
    registry.add(mine, world::Building{world::BuildingType::Mine, {8, 0}});
    registry.add(mine, world::Position{{8.5f, 0.5f}});
    const core::EntityId miner = world::spawn_worker(registry, "Worker", {0.5f, 0.5f});
    world::spawn_worker(registry, "Worker", {0.5f, 2.5f});
    *registry.get<world::Worker>(miner) = {world::WorkerState::Working, mine};
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    bool miner_label = false;
    bool idle_label = false;
    for (const DrawnText& text : canvas.texts) {
        miner_label = miner_label || text.text == "Worker (Miner)";
        idle_label = idle_label || text.text == "Worker (Idle)";
    }
    EXPECT_TRUE(miner_label);
    EXPECT_TRUE(idle_label);
}

TEST_F(EntityRendererTexture, AFinishedHeadquartersIsDrawnAsItsSpriteOnItsFootprint) {
    // Given: the headquarters sheet loads, 1 s of animation has passed
    const graphics::GameSprites sprites = sprites_with("buildings/headquarters", headquarters());
    core::Registry registry;
    const core::EntityId hq = registry.create("Headquarters");
    registry.add(hq, world::Building{world::BuildingType::Headquarters, {0, 0}, {5, 5}});
    registry.add(hq, world::Position{{0.0f, 0.0f}});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera, sprites, [] { return 1.0f; });
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: no cube, the anchor on the screen centre, and the current frame (the 5th)
    EXPECT_TRUE(canvas.triangles.empty());
    ASSERT_EQ(canvas.textures.size(), 1u);
    const DrawnTexture& drawn = canvas.textures.front();
    ASSERT_TRUE(drawn.options.source);
    EXPECT_FLOAT_EQ(drawn.dest.x + 168.0f, Screen.width / 2.0f);
    EXPECT_FLOAT_EQ(drawn.dest.y + 264.0f, Screen.height / 2.0f);
    EXPECT_FLOAT_EQ(drawn.options.source->x, 4.0f * 336.0f);
    EXPECT_EQ(canvas.texts.size(), 1u); // the label is still shown
}

TEST_F(EntityRendererTexture, ACharacterIsDrawnAsTheFrameOfItsFacingAndState) {
    // Given: the character sheet loads; a character walking, facing west
    const graphics::GameSprites sprites = sprites_with(graphics::CharacterSpriteName, character());
    core::Registry registry;
    const core::EntityId id = world::spawn_character(registry, "Player", {0.0f, 0.0f});
    registry.get<world::Facing>(id)->direction = world::Direction::West;
    registry.get<world::Character>(id)->state = world::CharacterState::Moving;
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera, sprites, [] { return 0.25f; });
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: no cube; row 6 (west), walk frame 1 + 2, feet on the screen centre
    EXPECT_TRUE(canvas.triangles.empty());
    ASSERT_EQ(canvas.textures.size(), 1u);
    const DrawnTexture& drawn = canvas.textures.front();
    ASSERT_TRUE(drawn.options.source);
    EXPECT_FLOAT_EQ(drawn.options.source->x, 3.0f * 32.0f);
    EXPECT_FLOAT_EQ(drawn.options.source->y, 6.0f * 48.0f);
    EXPECT_FLOAT_EQ(drawn.dest.x + 16.0f, Screen.width / 2.0f);
    EXPECT_FLOAT_EQ(drawn.dest.y + 44.0f, Screen.height / 2.0f);
    EXPECT_EQ(canvas.texts.size(), 1u); // the label is still shown
}

TEST_F(EntityRendererTexture, ASpriteWhoseTextureDoesNotLoadStaysACube) {
    // Given: the sheet file is missing
    graphics::SpriteLibrary library;
    library.add("buildings/headquarters", headquarters(),
                graphics::TextureManager::instance().register_texture("hq", "missing.png"));
    const graphics::GameSprites sprites(std::move(library));
    core::Registry registry;
    const core::EntityId hq = registry.create("Headquarters");
    registry.add(hq, world::Building{world::BuildingType::Headquarters, {0, 0}, {5, 5}});
    registry.add(hq, world::Position{{0.0f, 0.0f}});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera, sprites);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    EXPECT_TRUE(canvas.textures.empty());
    EXPECT_EQ(canvas.triangles.size(), TrianglesPerCube);
}

TEST(EntityRenderer, EntitiesOffScreenAreNotDrawnAtAll) {
    // Given: one entity under the camera, one 100 tiles away
    core::Registry registry;
    world::spawn_character(registry, "Here", {0.0f, 0.0f});
    world::spawn_character(registry, "Far away", {100.0f, 0.0f});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: neither its cube nor its label
    EXPECT_EQ(canvas.triangles.size(), TrianglesPerCube);
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts.front().text, "Here (Idle)");
}

TEST(EntityRenderer, AnEntityWhoseGroundPointIsOffScreenIsDrawnWhenItsBodyReachesIn) {
    // Given: a 5x5 building at world (12, 12): its centre is 384 px below the screen centre,
    // past the bottom edge, but its footprint rises 80 px above that
    core::Registry registry;
    const core::EntityId hq = registry.create("Headquarters");
    registry.add(hq, world::Building{world::BuildingType::Headquarters, {0, 0}, {5, 5}});
    registry.add(hq, world::Position{{12.0f, 12.0f}});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    EXPECT_EQ(canvas.triangles.size(), TrianglesPerCube);
}

TEST_F(EntityRendererTexture, AHeadquartersStillBeingBuiltStaysACube) {
    // Given
    const graphics::GameSprites sprites = sprites_with("buildings/headquarters", headquarters());
    core::Registry registry;
    const core::EntityId hq = registry.create("Headquarters");
    registry.add(hq, world::Building{world::BuildingType::Headquarters, {0, 0}, {5, 5}});
    registry.add(hq, world::ConstructionSite{0.0f, 100.0f});
    registry.add(hq, world::Position{{0.0f, 0.0f}});
    const world::Camera camera;
    const scene::world::EntityRenderer renderer(registry, camera, sprites, [] { return 1.0f; });
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    EXPECT_TRUE(canvas.textures.empty());
    EXPECT_EQ(canvas.triangles.size(), TrianglesPerCube);
}
