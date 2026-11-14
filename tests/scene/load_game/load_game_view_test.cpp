#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "event/event_bus.hpp"
#include "game/game_events.hpp"
#include "scene/load_game/load_game_view.hpp"
#include "support/fake_canvas.hpp"

namespace {

constexpr float ScreenWidth = 1280.0f;

// Records the business events the Load Game screen publishes.
struct EventRecorder {
    explicit EventRecorder(event::EventBus& bus) {
        subscriptions.push_back(event::subscribe_scoped<game::LoadSaveRequested>(
            bus, [this](const game::LoadSaveRequested& e) { loads.push_back(e.name); }));
        subscriptions.push_back(event::subscribe_scoped<game::BackToMenuRequested>(
            bus, [this](const game::BackToMenuRequested&) { ++back_requests; }));
    }

    std::vector<std::string> loads;
    int back_requests = 0;
    std::vector<event::ScopedSubscription> subscriptions;
};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(LoadGameView, ClickingASavePublishesLoadSaveRequestedWithItsName) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    scene::load_game::LoadGameView view(bus, {{"first", 300}, {"second", 200}, {"third", 100}});
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view.save_buttons()[1]->bounds()), true);

    // Then
    ASSERT_EQ(recorder.loads.size(), 1u);
    EXPECT_EQ(recorder.loads[0], "second");
}

TEST(LoadGameView, KeyboardStartsOnTheFirstSaveOfTheList) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    scene::load_game::LoadGameView view(bus, {{"newest", 300}, {"older", 200}});

    // When
    view.focus_group().activate_focused();

    // Then
    ASSERT_EQ(recorder.loads.size(), 1u);
    EXPECT_EQ(recorder.loads[0], "newest");
}

TEST(LoadGameView, WithoutSavesShowsAMessageAndOnlyBackWorks) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);

    // When
    scene::load_game::LoadGameView view(bus, {});
    view.focus_group().activate_focused();

    // Then
    EXPECT_TRUE(view.save_buttons().empty());
    ASSERT_NE(view.empty_message(), nullptr);
    EXPECT_FALSE(view.empty_message()->text().empty());
    EXPECT_EQ(recorder.back_requests, 1);
    EXPECT_TRUE(recorder.loads.empty());
}

TEST(LoadGameView, WithSavesThereIsNoEmptyMessage) {
    // Given
    event::EventBus bus;

    // When
    scene::load_game::LoadGameView view(bus, {{"base", 100}});

    // Then
    EXPECT_EQ(view.empty_message(), nullptr);
    EXPECT_EQ(view.save_buttons().size(), 1u);
}
