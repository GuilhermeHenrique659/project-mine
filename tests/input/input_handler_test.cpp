#include <gtest/gtest.h>

#include "input/input_handler.hpp"
#include "support/fake_keyboard.hpp"

using input::Action;
using input::InputHandler;
using input::InputMap;
using input::Key;


TEST(InputHandler, PressedKeyRunsTheHandlerOfItsAction) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::Enter, Action::Confirm);
    InputHandler handler(keyboard, map);
    int confirmations = 0;
    handler.on(Action::Confirm, [&] { ++confirmations; });
    keyboard.pressed = {Key::Enter};

    // When
    handler.update();

    // Then
    EXPECT_EQ(confirmations, 1);
}

TEST(InputHandler, UnboundKeyTriggersNothing) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::Enter, Action::Confirm);
    InputHandler handler(keyboard, map);
    int confirmations = 0;
    handler.on(Action::Confirm, [&] { ++confirmations; });
    keyboard.pressed = {Key::Space};

    // When
    handler.update();

    // Then
    EXPECT_EQ(confirmations, 0);
}

TEST(InputHandler, SeveralKeysCanTriggerTheSameAction) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::Up, Action::NavigateUp).bind(Key::W, Action::NavigateUp);
    InputHandler handler(keyboard, map);
    int moves = 0;
    handler.on(Action::NavigateUp, [&] { ++moves; });

    // When
    keyboard.pressed = {Key::Up};
    handler.update();
    keyboard.pressed = {Key::W};
    handler.update();

    // Then
    EXPECT_EQ(moves, 2);
}

TEST(InputHandler, ActionPressedThroughTwoKeysRunsOnce) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::Up, Action::NavigateUp).bind(Key::W, Action::NavigateUp);
    InputHandler handler(keyboard, map);
    int moves = 0;
    handler.on(Action::NavigateUp, [&] { ++moves; });
    keyboard.pressed = {Key::Up, Key::W};

    // When
    handler.update();

    // Then
    EXPECT_EQ(moves, 1);
}

TEST(InputHandler, RebindingAKeyMovesItToTheNewAction) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::Enter, Action::Confirm);
    InputHandler handler(keyboard, map);
    int confirmations = 0;
    int cancellations = 0;
    handler.on(Action::Confirm, [&] { ++confirmations; });
    handler.on(Action::Cancel, [&] { ++cancellations; });
    handler.map().bind(Key::Enter, Action::Cancel);
    keyboard.pressed = {Key::Enter};

    // When
    handler.update();

    // Then
    EXPECT_EQ(confirmations, 0);
    EXPECT_EQ(cancellations, 1);
}

TEST(InputHandler, TriggeredActionWithoutHandlerIsIgnored) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::Enter, Action::Confirm).bind(Key::Escape, Action::Cancel);
    InputHandler handler(keyboard, map);
    int cancellations = 0;
    handler.on(Action::Cancel, [&] { ++cancellations; });
    keyboard.pressed = {Key::Enter, Key::Escape};

    // When
    handler.update();

    // Then
    EXPECT_EQ(cancellations, 1);
}

TEST(InputHandler, MenuDefaultsMapArrowsWasdEnterSpaceAndEscape) {
    // Given
    FakeKeyboard keyboard;
    InputHandler handler(keyboard, InputMap::menu_defaults());
    int up = 0;
    int down = 0;
    int confirm = 0;
    int cancel = 0;
    handler.on(Action::NavigateUp, [&] { ++up; });
    handler.on(Action::NavigateDown, [&] { ++down; });
    handler.on(Action::Confirm, [&] { ++confirm; });
    handler.on(Action::Cancel, [&] { ++cancel; });

    // When
    for (const Key key : {Key::Up, Key::W, Key::Down, Key::S, Key::Enter, Key::Space, Key::Escape}) {
        keyboard.pressed = {key};
        handler.update();
    }

    // Then
    EXPECT_EQ(up, 2);
    EXPECT_EQ(down, 2);
    EXPECT_EQ(confirm, 2);
    EXPECT_EQ(cancel, 1);
}

TEST(InputHandler, TextEntryDefaultsLeaveLettersAndSpaceFreeForTyping) {
    // Given
    FakeKeyboard keyboard;
    InputHandler handler(keyboard, InputMap::text_entry_defaults());
    int triggered = 0;
    for (const Action action : {Action::NavigateUp, Action::NavigateDown, Action::NavigateLeft,
                                Action::NavigateRight, Action::Confirm, Action::Cancel,
                                Action::Erase}) {
        handler.on(action, [&] { ++triggered; });
    }

    // When
    keyboard.pressed = {Key::W, Key::A, Key::S, Key::D, Key::Space};
    handler.update();

    // Then
    EXPECT_EQ(triggered, 0);
}

TEST(InputHandler, TextEntryDefaultsMapArrowsEnterEscapeAndBackspace) {
    // Given
    FakeKeyboard keyboard;
    InputHandler handler(keyboard, InputMap::text_entry_defaults());
    int up = 0;
    int down = 0;
    int confirm = 0;
    int cancel = 0;
    int erase = 0;
    handler.on(Action::NavigateUp, [&] { ++up; });
    handler.on(Action::NavigateDown, [&] { ++down; });
    handler.on(Action::Confirm, [&] { ++confirm; });
    handler.on(Action::Cancel, [&] { ++cancel; });
    handler.on(Action::Erase, [&] { ++erase; });

    // When
    for (const Key key : {Key::Up, Key::Down, Key::Enter, Key::Escape, Key::Backspace}) {
        keyboard.pressed = {key};
        handler.update();
    }

    // Then
    EXPECT_EQ(up, 1);
    EXPECT_EQ(down, 1);
    EXPECT_EQ(confirm, 1);
    EXPECT_EQ(cancel, 1);
    EXPECT_EQ(erase, 1);
}
