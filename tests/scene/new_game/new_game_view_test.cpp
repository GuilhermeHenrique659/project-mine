#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "event/event_bus.hpp"
#include "game/game_events.hpp"
#include "scene/new_game/new_game_controller.hpp"
#include "scene/new_game/new_game_view.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_keyboard.hpp"
#include "support/fake_pointer.hpp"

namespace {

constexpr float ScreenWidth = 1280.0f;

// Records the business events the New Game screen publishes.
struct EventRecorder {
    explicit EventRecorder(event::EventBus& bus) {
        subscriptions.push_back(event::subscribe_scoped<game::CreateGameRequested>(
            bus, [this](const game::CreateGameRequested& e) { created.push_back(e.name); }));
        subscriptions.push_back(event::subscribe_scoped<game::BackToMenuRequested>(
            bus, [this](const game::BackToMenuRequested&) { ++back_requests; }));
    }

    std::vector<std::string> created;
    int back_requests = 0;
    std::vector<event::ScopedSubscription> subscriptions;
};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

void type(scene::new_game::NewGameView& view, const std::string& text) {
    for (const char character : text) view.type_char(character);
}

// A controller driven by a keyboard the test controls.
struct ControllerFixture {
    ControllerFixture() : recorder(bus), view(bus) {
        auto owned = std::make_unique<FakeKeyboard>();
        keyboard = owned.get();
        controller =
            std::make_unique<scene::new_game::NewGameController>(view, bus, std::move(owned),
                                                                  std::make_shared<FakePointer>());
    }

    void press(input::Key key) {
        keyboard->pressed = {key};
        controller->handle_keyboard();
        keyboard->pressed.clear();
    }

    event::EventBus bus;
    EventRecorder recorder;
    scene::new_game::NewGameView view;
    FakeKeyboard* keyboard = nullptr;
    std::unique_ptr<scene::new_game::NewGameController> controller;
};

} // namespace

TEST(NewGameView, CreateButtonPublishesTheTypedName) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    scene::new_game::NewGameView view(bus);
    view.layout(canvas, ScreenWidth);
    type(view, "My Base");

    // When
    view.handle_pointer(center_of(view.create_button().bounds()), true);

    // Then
    ASSERT_EQ(recorder.created.size(), 1u);
    EXPECT_EQ(recorder.created[0], "My Base");
}

TEST(NewGameView, SurroundingSpacesAreRemovedFromThePublishedName) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    scene::new_game::NewGameView view(bus);
    type(view, "  base ");

    // When: Enter with the name field focused
    view.focus_group().activate_focused();

    // Then
    ASSERT_EQ(recorder.created.size(), 1u);
    EXPECT_EQ(recorder.created[0], "base");
}

TEST(NewGameView, EmptyNamePublishesNothingAndShowsAMessage) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    scene::new_game::NewGameView view(bus);
    type(view, "   ");

    // When
    view.focus_group().activate_focused();

    // Then
    EXPECT_TRUE(recorder.created.empty());
    EXPECT_FALSE(view.error_message().empty());
}

TEST(NewGameView, NameUnusableAsAFileNamePublishesNothingAndShowsAMessage) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    scene::new_game::NewGameView view(bus);
    type(view, "a/b");

    // When
    view.focus_group().activate_focused();

    // Then
    EXPECT_TRUE(recorder.created.empty());
    EXPECT_FALSE(view.error_message().empty());
}

TEST(NewGameView, MessageDisappearsWhenThePlayerEditsTheName) {
    // Given
    event::EventBus bus;
    scene::new_game::NewGameView view(bus);
    view.focus_group().activate_focused();
    const bool shown_before = !view.error_message().empty();

    // When
    view.type_char('a');

    // Then
    EXPECT_TRUE(shown_before);
    EXPECT_TRUE(view.error_message().empty());
}

TEST(NewGameView, ClickingOutsideTheFieldDoesNotStopTyping) {
    // Given
    event::EventBus bus;
    FakeCanvas canvas;
    scene::new_game::NewGameView view(bus);
    view.layout(canvas, ScreenWidth);
    type(view, "ab");

    // When: a click on empty screen space
    view.handle_pointer({1.0f, 1.0f}, true);
    type(view, "cd");

    // Then
    EXPECT_EQ(view.name_input().value(), "abcd");
}

TEST(NewGameView, BackButtonPublishesBackToMenuRequested) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    scene::new_game::NewGameView view(bus);
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view.back_button().bounds()), true);

    // Then
    EXPECT_EQ(recorder.back_requests, 1);
    EXPECT_TRUE(recorder.created.empty());
}

TEST(NewGameController, TypedLettersAndSpacesGoToTheNameInsteadOfNavigating) {
    // Given
    ControllerFixture fixture;
    fixture.keyboard->type("was d");
    // The physical keys of those characters are down in the same frame.
    fixture.keyboard->pressed = {input::Key::W, input::Key::A, input::Key::S, input::Key::D,
                                 input::Key::Space};

    // When
    fixture.controller->handle_keyboard();

    // Then: nothing was activated and the focus never left the field
    EXPECT_EQ(fixture.view.name_input().value(), "was d");
    EXPECT_TRUE(fixture.view.name_input().focused());
    EXPECT_TRUE(fixture.recorder.created.empty());
    EXPECT_TRUE(fixture.view.error_message().empty());
}

TEST(NewGameController, BackspaceErasesTheLastCharacter) {
    // Given
    ControllerFixture fixture;
    fixture.keyboard->type("base");
    fixture.controller->handle_keyboard();

    // When
    fixture.press(input::Key::Backspace);

    // Then
    EXPECT_EQ(fixture.view.name_input().value(), "bas");
}

TEST(NewGameController, EnterCreatesTheGameWithTheTypedName) {
    // Given
    ControllerFixture fixture;
    fixture.keyboard->type("base");
    fixture.controller->handle_keyboard();

    // When
    fixture.press(input::Key::Enter);

    // Then
    ASSERT_EQ(fixture.recorder.created.size(), 1u);
    EXPECT_EQ(fixture.recorder.created[0], "base");
}

TEST(NewGameController, EscapeGoesBackToTheMenuWithoutCreating) {
    // Given
    ControllerFixture fixture;
    fixture.keyboard->type("base");
    fixture.controller->handle_keyboard();

    // When
    fixture.press(input::Key::Escape);

    // Then
    EXPECT_EQ(fixture.recorder.back_requests, 1);
    EXPECT_TRUE(fixture.recorder.created.empty());
}
