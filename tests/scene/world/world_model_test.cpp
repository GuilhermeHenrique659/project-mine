#include <gtest/gtest.h>

#include "game/save_game.hpp"
#include "scene/world/world_model.hpp"
#include "world/components/position.hpp"

namespace {

::world::WorldPoint player_position(const scene::world::WorldModel& model) {
    const auto* position = model.registry().get<::world::Position>(model.player_id());
    return position == nullptr ? ::world::WorldPoint{-999.0f, -999.0f} : position->point;
}

} // namespace

TEST(WorldModel, PlayerStartsAtTheWorldOriginAndCanBeFound) {
    // Given / When
    event::EventBus bus;
    const scene::world::WorldModel model(bus, game::SaveGame{"Alpha", {}});

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
    scene::world::WorldModel model(bus, game::SaveGame{"Alpha", {}});
    model.set_player_direction({1.0f, 0.0f});

    // When
    model.update(0.5f);

    // Then
    EXPECT_GT(player_position(model).x, 0.0f);
    EXPECT_FLOAT_EQ(player_position(model).y, 0.0f);
}

TEST(WorldModel, ClearingTheDirectionStopsThePlayer) {
    // Given
    event::EventBus bus;
    scene::world::WorldModel model(bus, game::SaveGame{"Alpha", {}});
    model.set_player_direction({0.0f, 1.0f});
    model.update(0.5f);
    const ::world::WorldPoint stopped_at = player_position(model);

    // When
    model.set_player_direction({0.0f, 0.0f});
    model.update(0.5f);

    // Then
    EXPECT_GT(stopped_at.y, 0.0f);
    EXPECT_FLOAT_EQ(player_position(model).x, stopped_at.x);
    EXPECT_FLOAT_EQ(player_position(model).y, stopped_at.y);
}
