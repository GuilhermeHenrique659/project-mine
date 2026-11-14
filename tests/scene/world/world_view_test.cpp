#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "event/event_bus.hpp"
#include "game/game_events.hpp"
#include "scene/world/world_model.hpp"
#include "scene/world/world_view.hpp"
#include "support/fake_canvas.hpp"

namespace {

constexpr float ScreenWidth = 1280.0f;
constexpr float ScreenHeight = 720.0f;

// Records the business events the world screen publishes.
struct EventRecorder {
    explicit EventRecorder(event::EventBus& bus) {
        subscriptions.push_back(event::subscribe_scoped<game::SaveGameRequested>(
            bus, [this](const game::SaveGameRequested& e) { saved = e.save; }));
        subscriptions.push_back(event::subscribe_scoped<game::BackToMenuRequested>(
            bus, [this](const game::BackToMenuRequested&) { ++back_requests; }));
    }

    std::optional<game::SaveGame> saved;
    int back_requests = 0;
    std::vector<event::ScopedSubscription> subscriptions;
};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(WorldView, SaveButtonPublishesTheStateOfTheGameBeingPlayed) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    scene::world::WorldModel model(bus, game::SaveGame{"My Base", 1'700'000'000});
    model.toggle_pause();
    scene::world::WorldView view(bus, model);
    view.layout(canvas, {ScreenWidth, ScreenHeight});

    // When
    view.handle_pointer(center_of(view.menu().save_button().bounds()), true);

    // Then
    ASSERT_TRUE(recorder.saved.has_value());
    EXPECT_EQ(*recorder.saved, (game::SaveGame{"My Base", 1'700'000'000}));
    EXPECT_EQ(recorder.back_requests, 0);
}

TEST(WorldView, MainMenuButtonGoesBackWithoutSaving) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    scene::world::WorldModel model(bus, game::SaveGame{"My Base", 1'700'000'000});
    model.toggle_pause();
    scene::world::WorldView view(bus, model);
    view.layout(canvas, {ScreenWidth, ScreenHeight});

    // When
    view.handle_pointer(center_of(view.menu().menu_button().bounds()), true);

    // Then
    EXPECT_EQ(recorder.back_requests, 1);
    EXPECT_FALSE(recorder.saved.has_value());
}

TEST(WorldView, ShowsTheNameOfTheLoadedSave) {
    // Given
    event::EventBus bus;
    const scene::world::WorldModel model(bus, game::SaveGame{"My Base", 1'700'000'000});

    // When
    scene::world::WorldView view(bus, model);

    // Then
    EXPECT_EQ(view.menu().name_label().text(), "My Base");
}

TEST(WorldView, MenuIgnoresClicksWhileThePlayIsNotPaused) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    scene::world::WorldModel model(bus, game::SaveGame{"My Base", 1'700'000'000});
    scene::world::WorldView view(bus, model);
    view.layout(canvas, {ScreenWidth, ScreenHeight});

    // When
    view.handle_pointer(center_of(view.menu().menu_button().bounds()), true);

    // Then
    EXPECT_EQ(recorder.back_requests, 0);
}

TEST(WorldView, MenuIsDrawnOnlyWhilePaused) {
    // Given
    event::EventBus bus;
    scene::world::WorldModel model(bus, game::SaveGame{"My Base", 1'700'000'000});
    const scene::world::WorldView view(bus, model);
    FakeCanvas hidden;
    FakeCanvas shown;

    // When
    view.draw_menu(hidden, {ScreenWidth, ScreenHeight});
    model.toggle_pause();
    view.draw_menu(shown, {ScreenWidth, ScreenHeight});

    // Then
    EXPECT_TRUE(hidden.texts.empty());
    EXPECT_FALSE(shown.texts.empty());
}

TEST(WorldView, CharacterIsDrawnAsACubeWithItsLabelAndStateAroundTheScreenCentre) {
    // Given
    event::EventBus bus;
    scene::world::WorldModel model(bus, game::SaveGame{"Alpha", 0});
    model.set_player_direction({1.0f, 0.0f});
    model.update(0.5f); // the player walks 2 tiles along world x
    const scene::world::WorldView view(bus, model);
    FakeCanvas canvas;

    // When
    view.draw_entities(canvas, {1280.0f, 720.0f});

    // Then: world (2, 0) with 64x32 tiles is (64, 32) pixels from the screen centre
    ASSERT_EQ(canvas.fills.size(), 2u); // top face, then front face
    const ui::Point marker_center = center_of(canvas.fills.back().rect);
    EXPECT_FLOAT_EQ(marker_center.x, 640.0f + 64.0f);
    EXPECT_FLOAT_EQ(marker_center.y, 360.0f + 32.0f);
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts.front().text, "Player (Moving)");
    EXPECT_LT(canvas.texts.front().position.y, canvas.fills.front().rect.y);
}

TEST(WorldView, MenuIsCenteredOnTheScreen) {
    // Given
    event::EventBus bus;
    FakeCanvas canvas;
    scene::world::WorldModel model(bus, game::SaveGame{"Alpha", 0});
    model.toggle_pause();
    scene::world::WorldView view(bus, model);

    // When
    view.layout(canvas, {ScreenWidth, ScreenHeight});

    // Then: the buttons are centred horizontally and the whole panel vertically
    const ui::Rect name = view.menu().name_label().bounds();
    const ui::Rect last = view.menu().menu_button().bounds();
    EXPECT_NEAR(center_of(view.menu().save_button().bounds()).x, ScreenWidth / 2.0f, 1.0f);
    EXPECT_NEAR((name.y + last.y + last.height) / 2.0f, ScreenHeight / 2.0f, 40.0f);
}
