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
    const scene::world::WorldModel model(game::SaveGame{"My Base", 1'700'000'000});
    scene::world::WorldView view(bus, model);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view.save_button().bounds()), true);

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
    const scene::world::WorldModel model(game::SaveGame{"My Base", 1'700'000'000});
    scene::world::WorldView view(bus, model);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view.menu_button().bounds()), true);

    // Then
    EXPECT_EQ(recorder.back_requests, 1);
    EXPECT_FALSE(recorder.saved.has_value());
}

TEST(WorldView, ShowsTheNameOfTheLoadedSave) {
    // Given
    event::EventBus bus;
    const scene::world::WorldModel model(game::SaveGame{"My Base", 1'700'000'000});

    // When
    scene::world::WorldView view(bus, model);

    // Then
    EXPECT_EQ(view.name_label().text(), "My Base");
}
