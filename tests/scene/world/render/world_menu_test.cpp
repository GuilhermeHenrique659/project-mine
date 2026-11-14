#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

#include "event/event_bus.hpp"
#include "game/game_events.hpp"
#include "scene/world/render/world_menu.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

// Records the business events the pause menu publishes.
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

// A paused world with its menu laid out, ready to be clicked through a fake pointer.
struct PausedMenu {
    PausedMenu() : model(bus, input.handler, game::SaveGame{"My Base", 1'700'000'000}) {
        model.toggle_pause();
        menu = std::make_unique<scene::world::WorldMenu>(bus, model, pointer);
        menu->layout(canvas, Screen);
    }

    void point_at(const ui::Rect& bounds, bool release) {
        const ui::Point at = center_of(bounds);
        pointer->at = {at.x, at.y};
        pointer->release = release;
        menu->update();
    }

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    std::unique_ptr<scene::world::WorldMenu> menu;
};

} // namespace

TEST(WorldMenu, IsVisibleOnlyWhileTheGameIsPaused) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, game::SaveGame{"Alpha", 0});
    const scene::world::WorldMenu menu(bus, model, std::make_shared<FakePointer>());

    // When
    const bool visible_while_playing = menu.visible();
    model.toggle_pause();

    // Then
    EXPECT_FALSE(visible_while_playing);
    EXPECT_TRUE(menu.visible());
}

TEST(WorldMenu, ClickingSavePublishesTheStateOfTheGameBeingPlayed) {
    // Given
    PausedMenu paused;
    EventRecorder recorder(paused.bus);

    // When
    paused.point_at(paused.menu->save_button().bounds(), true);

    // Then
    ASSERT_TRUE(recorder.saved.has_value());
    EXPECT_EQ(*recorder.saved, paused.model.save_game());
    EXPECT_EQ(recorder.saved->name, "My Base");
    EXPECT_EQ(recorder.saved->created_at, 1'700'000'000);
    EXPECT_EQ(recorder.back_requests, 0);
}

TEST(WorldMenu, SaveCapturesThePlayerPositionAtClickTime) {
    // Given
    PausedMenu paused;
    EventRecorder recorder(paused.bus);
    paused.model.toggle_pause();
    paused.input.keyboard.down = {input::Key::S, input::Key::D}; // world +x
    paused.model.update(0.5f);
    paused.model.toggle_pause();

    // When
    paused.point_at(paused.menu->save_button().bounds(), true);

    // Then
    ASSERT_TRUE(recorder.saved.has_value());
    ASSERT_EQ(recorder.saved->entities.size(), 1u);
    EXPECT_GT(std::stof(recorder.saved->entities[0].fields.at("position.x")), 0.0f);
}

TEST(WorldMenu, ClickingMainMenuGoesBackWithoutSaving) {
    // Given
    PausedMenu paused;
    EventRecorder recorder(paused.bus);

    // When
    paused.point_at(paused.menu->menu_button().bounds(), true);

    // Then
    EXPECT_EQ(recorder.back_requests, 1);
    EXPECT_FALSE(recorder.saved.has_value());
}

TEST(WorldMenu, PointingAtAButtonWithoutReleasingItDoesNothing) {
    // Given
    PausedMenu paused;
    EventRecorder recorder(paused.bus);

    // When
    paused.point_at(paused.menu->save_button().bounds(), false);

    // Then
    EXPECT_FALSE(recorder.saved.has_value());
    EXPECT_EQ(recorder.back_requests, 0);
}

TEST(WorldMenu, ShowsTheNameOfTheLoadedSave) {
    // Given / When
    PausedMenu paused;

    // Then
    EXPECT_EQ(paused.menu->name_label().text(), "My Base");
}

TEST(WorldMenu, IsCenteredOnTheScreen) {
    // Given / When
    PausedMenu paused;

    // Then: the buttons are centred horizontally and the whole panel vertically
    const ui::Rect name = paused.menu->name_label().bounds();
    const ui::Rect last = paused.menu->menu_button().bounds();
    EXPECT_NEAR(center_of(paused.menu->save_button().bounds()).x, Screen.width / 2.0f, 1.0f);
    EXPECT_NEAR((name.y + last.y + last.height) / 2.0f, Screen.height / 2.0f, 40.0f);
}
