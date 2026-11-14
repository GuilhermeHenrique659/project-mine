#include <gtest/gtest.h>

#include <vector>

#include "event/event_bus.hpp"
#include "fake_canvas.hpp"
#include "game/game_events.hpp"
#include "scene/menu/menu_view.hpp"

namespace {

constexpr float ScreenWidth = 1280.0f;
constexpr std::size_t NewGameIndex = 0;
constexpr std::size_t LoadGameIndex = 1;
constexpr std::size_t QuitIndex = 2;

// Counts the business events the menu publishes.
struct EventCounter {
    explicit EventCounter(event::EventBus& bus) {
        subscriptions.push_back(event::subscribe_scoped<game::NewGameRequested>(
            bus, [this](const game::NewGameRequested&) { ++new_games; }));
        subscriptions.push_back(event::subscribe_scoped<game::LoadGameRequested>(
            bus, [this](const game::LoadGameRequested&) { ++loads; }));
        subscriptions.push_back(event::subscribe_scoped<game::QuitRequested>(
            bus, [this](const game::QuitRequested&) { ++quits; }));
    }

    int new_games = 0;
    int loads = 0;
    int quits = 0;
    std::vector<event::ScopedSubscription> subscriptions;
};

ui::Point center_of(const scene::menu::MenuView& view, std::size_t button_index) {
    const ui::Rect& rect = view.buttons()[button_index]->bounds();
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(MenuView, ClickingNewGameButtonPublishesNewGameRequested) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    FakeCanvas canvas;
    scene::menu::MenuView view(bus);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view, NewGameIndex), true);

    // Then
    EXPECT_EQ(counter.new_games, 1);
    EXPECT_EQ(counter.loads, 0);
    EXPECT_EQ(counter.quits, 0);
}

TEST(MenuView, ClickingLoadGameButtonPublishesLoadGameRequested) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    FakeCanvas canvas;
    scene::menu::MenuView view(bus);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view, LoadGameIndex), true);

    // Then
    EXPECT_EQ(counter.loads, 1);
    EXPECT_EQ(counter.new_games, 0);
    EXPECT_EQ(counter.quits, 0);
}

TEST(MenuView, ClickingQuitButtonPublishesQuitRequested) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    FakeCanvas canvas;
    scene::menu::MenuView view(bus);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view, QuitIndex), true);

    // Then
    EXPECT_EQ(counter.quits, 1);
    EXPECT_EQ(counter.new_games, 0);
    EXPECT_EQ(counter.loads, 0);
}

TEST(MenuView, ClickingOutsideEveryButtonPublishesNothing) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    FakeCanvas canvas;
    scene::menu::MenuView view(bus);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer({5.0f, 5.0f}, true);

    // Then
    EXPECT_EQ(counter.new_games + counter.loads + counter.quits, 0);
}

TEST(MenuView, KeyboardFocusStartsOnFirstButton) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    scene::menu::MenuView view(bus);

    // When
    view.focus_group().activate_focused();

    // Then
    EXPECT_EQ(counter.new_games, 1);
}

TEST(MenuView, FocusNextMovesToTheFollowingButton) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    scene::menu::MenuView view(bus);

    // When
    view.focus_group().focus_next();
    view.focus_group().activate_focused();

    // Then
    EXPECT_EQ(counter.loads, 1);
}

TEST(MenuView, FocusNextWrapsFromLastToFirst) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    scene::menu::MenuView view(bus);

    // When
    for (std::size_t i = 0; i < view.buttons().size(); ++i) view.focus_group().focus_next();
    view.focus_group().activate_focused();

    // Then
    EXPECT_EQ(counter.new_games, 1);
}

TEST(MenuView, FocusPreviousWrapsFromFirstToLast) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    scene::menu::MenuView view(bus);

    // When
    view.focus_group().focus_previous();
    view.focus_group().activate_focused();

    // Then
    EXPECT_EQ(counter.quits, 1);
}

TEST(MenuView, HoveringAButtonMovesKeyboardFocusToIt) {
    // Given
    event::EventBus bus;
    EventCounter counter(bus);
    FakeCanvas canvas;
    scene::menu::MenuView view(bus);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view, QuitIndex), false);
    view.focus_group().activate_focused();

    // Then
    EXPECT_EQ(counter.quits, 1);
}
