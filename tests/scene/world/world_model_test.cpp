#include <gtest/gtest.h>

#include "game/save_game.hpp"
#include "scene/world/world_model.hpp"
#include "support/fake_input.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"

#include <optional>
#include <string>

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

TEST(WorldModel, TogglingThePlayerMenuOpensAndClosesIt) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    const bool open_at_start = model.is_player_menu_open();

    // When
    model.toggle_player_menu();
    const bool open_after_first = model.is_player_menu_open();
    model.toggle_player_menu();

    // Then
    EXPECT_FALSE(open_at_start);
    EXPECT_TRUE(open_after_first);
    EXPECT_FALSE(model.is_player_menu_open());
}

TEST(WorldModel, ThePlayerKeepsWalkingWhileThePlayerMenuIsOpen) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    model.toggle_player_menu();
    input.keyboard.down.insert(input::Key::D);

    // When
    model.update(0.5f);

    // Then
    const ::world::WorldPoint at = player_position(model);
    EXPECT_FALSE(at.x == 0.0f && at.y == 0.0f);
}

TEST(WorldModel, ThePlayerMenuDoesNotOpenWhilePaused) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    model.toggle_pause();

    // When
    model.toggle_player_menu();

    // Then
    EXPECT_FALSE(model.is_player_menu_open());
}

TEST(WorldModel, CancelClosesTheOpenPlayerMenuInsteadOfPausing) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    model.toggle_player_menu();

    // When
    model.cancel();

    // Then
    EXPECT_FALSE(model.is_player_menu_open());
    EXPECT_FALSE(model.is_paused());
}

TEST(WorldModel, CancelTogglesThePauseWhenThePlayerMenuIsClosed) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});

    // When
    model.cancel();
    const bool paused_after_first = model.is_paused();
    model.cancel();

    // Then
    EXPECT_TRUE(paused_after_first);
    EXPECT_FALSE(model.is_paused());
}

TEST(WorldModel, BuildBarOpensAndClosesWhenToggled) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});

    // When
    model.toggle_build_bar();
    const bool open_after_first = model.is_build_bar_open();
    model.toggle_build_bar();

    // Then
    EXPECT_TRUE(open_after_first);
    EXPECT_FALSE(model.is_build_bar_open());
}

TEST(WorldModel, BuildBarCannotBeOpenedWhilePaused) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    model.toggle_pause();

    // When
    model.toggle_build_bar();

    // Then
    EXPECT_FALSE(model.is_build_bar_open());
}

TEST(WorldModel, ARequestedHeadquartersIsBuiltAndSurvivesASave) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});
    const ::world::Construction& hq =
        *::world::find_construction(::world::BuildingType::Headquarters);
    const auto before = model.build_status(hq);

    // When
    model.request_build(::world::BuildingType::Headquarters, {3, 2});

    // Then
    EXPECT_EQ(before, ::world::BuildResult::Ok);
    EXPECT_EQ(model.build_status(hq), ::world::BuildResult::AlreadyExists);
    event::EventBus other_bus;
    const scene::world::WorldModel loaded(other_bus, input.handler, model.save_game());
    EXPECT_TRUE(::world::has_headquarters(loaded.registry()));
}

namespace {

std::optional<::world::TileCoord> find_ore(const ::world::Terrain& terrain) {
    for (int x = -60; x < 60; ++x) {
        for (int y = -60; y < 60; ++y) {
            if (::world::is_ore(terrain.tile_at({x, y}))) {
                return ::world::TileCoord{x, y};
            }
        }
    }
    return std::nullopt;
}

// A save whose player stands next to `tile` with 100 iron ore, and a headquarters.
game::SaveGame save_next_to(::world::TileCoord tile) {
    game::SaveGame save{"Alpha", {}};
    save.entities.push_back({"character", "Player",
                             {{"position.x", std::to_string(tile.x + 1.5f)},
                              {"position.y", std::to_string(tile.y + 0.5f)},
                              {"player", "1"},
                              {"inventory.0", "IronOre:100"}}});
    save.entities.push_back(
        {"building", "Headquarters", {{"building.type", "Headquarters"}, {"building.x", "0"}, {"building.y", "0"}}});
    return save;
}

float site_progress(const scene::world::WorldModel& model, ::world::TileCoord tile) {
    for (const core::Entity& entity : model.registry().entities()) {
        const auto* building = model.registry().get<::world::Building>(entity.id);
        const auto* site = model.registry().get<::world::ConstructionSite>(entity.id);
        if (building != nullptr && site != nullptr && building->tile == tile) {
            return site->progress;
        }
    }
    return -1.0f;
}

} // namespace

TEST(WorldModel, ANewPlayerStartsWith200IronOre) {
    // Given / When
    event::EventBus bus;
    FakeInput input;
    const scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", {}});

    // Then
    const auto* inventory = model.registry().get<::world::Inventory>(model.player_id());
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->count(::world::items::IronOre), 200);
}

TEST(WorldModel, ThePlayerBuildsTheSiteItWorksOnOnlyWhileWorking) {
    // Given
    event::EventBus bus;
    FakeInput input;
    const auto ore = find_ore(::world::Terrain(game::SaveGame{"Alpha", {}}.seed));
    ASSERT_TRUE(ore);
    scene::world::WorldModel model(bus, input.handler, save_next_to(*ore));
    model.request_build(::world::BuildingType::Mine, *ore);
    ASSERT_FLOAT_EQ(site_progress(model, *ore), 0.0f);

    // When
    model.work_on(*ore);
    model.update(1.0f);
    const float while_working = site_progress(model, *ore);
    model.work_on(std::nullopt);
    model.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(while_working, ::world::CharacterBuildRate);
    EXPECT_FLOAT_EQ(site_progress(model, *ore), ::world::CharacterBuildRate);
}

namespace {

// save_next_to(tile) plus a finished mine on `tile` holding 40 coal.
game::SaveGame save_with_mine(::world::TileCoord tile) {
    game::SaveGame save = save_next_to(tile);
    save.entities.push_back({"building", "Mine",
                             {{"building.type", "Mine"},
                              {"building.x", std::to_string(tile.x)},
                              {"building.y", std::to_string(tile.y)},
                              {"inventory.0", "Coal:40"}}});
    return save;
}

struct MineWorld {
    event::EventBus bus;
    FakeInput input;
    ::world::TileCoord ore = *find_ore(::world::Terrain(game::SaveGame{"Alpha", {}}.seed));
    scene::world::WorldModel model{bus, input.handler, save_with_mine(ore)};

    int coal(core::EntityId id) const {
        return model.registry().get<::world::Inventory>(id)->count(::world::items::Coal);
    }
};

} // namespace

TEST(WorldModel, OpeningTheTransferOnAMineInReachLetsItemsMoveToThePlayer) {
    // Given
    MineWorld w;

    // When
    w.model.open_transfer(w.ore);
    const core::EntityId mine = w.model.transfer_target();
    w.model.request_transfer(mine, w.model.player_id(), ::world::items::Coal, 40);

    // Then
    ASSERT_NE(mine, 0u);
    EXPECT_EQ(w.model.registry().get<::world::Building>(mine)->type, ::world::BuildingType::Mine);
    EXPECT_EQ(w.coal(mine), 0);
    EXPECT_EQ(w.coal(w.model.player_id()), 40);
}

TEST(WorldModel, TheTransferDoesNotOpenOnABuildingOutOfReachOrWithoutAnInventory) {
    // Given
    MineWorld w;

    // When: the headquarters on (0, 0) is far from the player and has no inventory
    w.model.open_transfer({0, 0});

    // Then
    EXPECT_EQ(w.model.transfer_target(), 0u);
}

TEST(WorldModel, WalkingOutOfReachClosesTheTransfer) {
    // Given
    MineWorld w;
    w.model.open_transfer(w.ore);
    w.input.keyboard.down = {input::Key::S, input::Key::D}; // walk away along world +x

    // When
    for (int frame = 0; frame < 10; ++frame) {
        w.model.update(0.1f);
    }

    // Then
    EXPECT_EQ(w.model.transfer_target(), 0u);
}

TEST(WorldModel, CancelClosesTheTransferBeforePausing) {
    // Given
    MineWorld w;
    w.model.open_transfer(w.ore);

    // When
    w.model.cancel();

    // Then
    EXPECT_EQ(w.model.transfer_target(), 0u);
    EXPECT_FALSE(w.model.is_paused());
}
