#include <gtest/gtest.h>

#include "game/save_game.hpp"
#include "scene/world/world_model.hpp"
#include "support/fake_input.hpp"
#include "scene/world/core/components/position.hpp"

namespace {

::world::WorldPoint player_position(const scene::world::WorldModel& model) {
    const auto* position = model.registry().get<::world::Position>(model.player_id());
    return position == nullptr ? ::world::WorldPoint{-999.0f, -999.0f} : position->point;
}

} // namespace

TEST(WorldModel, PlayerStartsAtTheWorldOriginAndCanBeFound) {
    // Given / When
    event::EventBus bus;
    FakeInput input;
    const scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});

    // Then
    const core::Entity* player = model.registry().find(model.player_id());
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->label, "Player");
    EXPECT_FLOAT_EQ(player_position(model).x, 0.0f);
    EXPECT_FLOAT_EQ(player_position(model).y, 0.0f);
}

TEST(WorldModel, UpdatingWithADirectionMovesThePlayer) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    input.keyboard.down = {input::Key::S, input::Key::D}; // screen down-right = world +x

    // When
    model.update(0.5f);

    // Then
    EXPECT_GT(player_position(model).x, 0.0f);
    EXPECT_FLOAT_EQ(player_position(model).y, 0.0f);
}

TEST(WorldModel, CameraFollowsThePlayerAsItMoves) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    input.keyboard.down = {input::Key::S, input::Key::D}; // screen down-right = world +x

    // When
    model.update(0.5f);

    // Then: the camera is on where the player is after this frame's movement
    ASSERT_GT(player_position(model).x, 0.0f);
    EXPECT_FLOAT_EQ(model.camera().center.x, player_position(model).x);
    EXPECT_FLOAT_EQ(model.camera().center.y, player_position(model).y);
}

TEST(WorldModel, TogglingPauseOpensAndClosesThePause) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    const bool paused_at_start = model.is_paused();

    // When
    model.toggle_pause();
    const bool paused_after_first = model.is_paused();
    model.toggle_pause();

    // Then
    EXPECT_FALSE(paused_at_start);
    EXPECT_TRUE(paused_after_first);
    EXPECT_FALSE(model.is_paused());
}

TEST(WorldModel, ThePlayerDoesNotMoveWhilePaused) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    input.keyboard.down = {input::Key::S, input::Key::D}; // screen down-right = world +x
    model.toggle_pause();

    // When
    model.update(0.5f);

    // Then
    EXPECT_FLOAT_EQ(player_position(model).x, 0.0f);
}

TEST(WorldModel, ClearingTheDirectionStopsThePlayer) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    input.keyboard.down = {input::Key::A, input::Key::S}; // screen down-left = world +y
    model.update(0.5f);
    const ::world::WorldPoint stopped_at = player_position(model);

    // When
    input.keyboard.down.clear();
    model.update(0.5f);

    // Then
    EXPECT_GT(stopped_at.y, 0.0f);
    EXPECT_FLOAT_EQ(player_position(model).x, stopped_at.x);
    EXPECT_FLOAT_EQ(player_position(model).y, stopped_at.y);
}

TEST(WorldModel, TerrainIsGeneratedFromTheSavedSeed) {
    // Given
    event::EventBus bus;
    FakeInput input;

    // When
    const scene::world::WorldModel first(bus, input.handler, game::SaveGame{"A", 0, 4242});
    const scene::world::WorldModel loaded(bus, input.handler, game::SaveGame{"A", 0, 4242});

    // Then: a loaded save recreates the same world
    EXPECT_EQ(first.terrain().seed(), 4242u);
    for (int y = -30; y <= 30; ++y) {
        for (int x = -30; x <= 30; ++x) {
            ASSERT_EQ(first.terrain().tile_at({x, y}), loaded.terrain().tile_at({x, y}));
        }
    }
}

namespace {

game::SaveGame save_with_player_at(float x, float y) {
    game::SaveGame save{"Alpha", 0, 1};
    save.entities = {{"character",
                      "Player",
                      {{"position.x", std::to_string(x)},
                       {"position.y", std::to_string(y)},
                       {"state", "Idle"},
                       {"player", "1"}}}};
    return save;
}

} // namespace

TEST(WorldModel, PlayerStartsWhereTheSaveLeftItWithTheCameraThere) {
    // Given
    event::EventBus bus;
    FakeInput input;
    const game::SaveGame save = save_with_player_at(7.0f, -3.0f);

    // When
    const scene::world::WorldModel model(bus, input.handler, save);

    // Then
    EXPECT_FLOAT_EQ(player_position(model).x, 7.0f);
    EXPECT_FLOAT_EQ(player_position(model).y, -3.0f);
    EXPECT_FLOAT_EQ(model.camera().center.x, 7.0f);
    EXPECT_FLOAT_EQ(model.camera().center.y, -3.0f);
}

TEST(WorldModel, SaveGameHoldsThePlayerCurrentPositionAndIdle) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, save_with_player_at(1.0f, 1.0f));
    input.keyboard.down = {input::Key::S, input::Key::D}; // world +x
    model.update(0.5f);

    // When
    const game::SaveGame saved = model.save_game();

    // Then: moved, yet saved as standing
    ASSERT_EQ(saved.entities.size(), 1u);
    EXPECT_GT(std::stof(saved.entities[0].fields.at("position.x")), 1.0f);
    EXPECT_EQ(saved.entities[0].fields.at("state"), "Idle");
    EXPECT_EQ(saved.name, "Alpha");
}

TEST(WorldModel, SaveWithoutEntitiesStillSpawnsThePlayerAtTheOrigin) {
    // Given
    event::EventBus bus;
    FakeInput input;
    const game::SaveGame save{"Alpha", 0, 1};

    // When
    const scene::world::WorldModel model(bus, input.handler, save);

    // Then
    EXPECT_EQ(model.registry().find(model.player_id())->label, "Player");
    EXPECT_FLOAT_EQ(player_position(model).x, 0.0f);
    EXPECT_EQ(model.save_game().entities.size(), 1u);
}

TEST(WorldModel, SaveWithoutAPlayerRecordGetsADefaultPlayer) {
    // Given
    game::SaveGame save{"Alpha", 0, 1};
    save.entities = {{"character", "Guard", {{"position.x", "5"}, {"position.y", "5"}}}};
    event::EventBus bus;
    FakeInput input;

    // When
    const scene::world::WorldModel model(bus, input.handler, save);

    // Then
    EXPECT_EQ(model.registry().find(model.player_id())->label, "Player");
    EXPECT_FLOAT_EQ(player_position(model).x, 0.0f);
}

TEST(WorldModel, ZoomingChangesTheCameraZoom) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});

    // When
    model.zoom_camera(-2.0f);

    // Then
    EXPECT_LT(model.camera().zoom, 1.0f);
}

TEST(WorldModel, ZoomingIsIgnoredWhilePaused) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    model.toggle_pause();

    // When
    model.zoom_camera(2.0f);

    // Then
    EXPECT_FLOAT_EQ(model.camera().zoom, 1.0f);
}
