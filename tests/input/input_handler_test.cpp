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

TEST(InputHandler, ActionIsHeldWhileABoundKeyIsDownAndNotAfterRelease) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::W, Action::MoveUp).bind(Key::Up, Action::MoveUp);
    InputHandler handler(keyboard, map);

    // When
    keyboard.down = {Key::Up};
    const bool held_while_down = handler.is_held(Action::MoveUp);
    keyboard.down.clear();
    const bool held_after_release = handler.is_held(Action::MoveUp);

    // Then
    EXPECT_TRUE(held_while_down);
    EXPECT_FALSE(held_after_release);
}

TEST(InputHandler, ActionIsNotHeldWhenOnlyAnUnboundKeyIsDown) {
    // Given
    FakeKeyboard keyboard;
    InputMap map;
    map.bind(Key::W, Action::MoveUp);
    InputHandler handler(keyboard, map);
    keyboard.down = {Key::S};

    // When
    const bool held = handler.is_held(Action::MoveUp);

    // Then
    EXPECT_FALSE(held);
}

TEST(InputHandler, WorldDefaultsMoveWithWasdAndKeepArrowsForTheMenuPanel) {
    // Given
    const InputMap map = InputMap::world_defaults();

    // When / Then
    EXPECT_EQ(map.action_for(Key::W), Action::MoveUp);
    EXPECT_EQ(map.action_for(Key::S), Action::MoveDown);
    EXPECT_EQ(map.action_for(Key::A), Action::MoveLeft);
    EXPECT_EQ(map.action_for(Key::D), Action::MoveRight);
    EXPECT_EQ(map.action_for(Key::Up), Action::NavigateUp);
    EXPECT_EQ(map.action_for(Key::Down), Action::NavigateDown);
    EXPECT_EQ(map.action_for(Key::Enter), Action::Confirm);
    EXPECT_EQ(map.action_for(Key::Space), Action::Confirm);
    EXPECT_EQ(map.action_for(Key::Escape), Action::Pause);
}

TEST(InputHandler, WorldDefaultsToggleTheInventoryWithTab) {
    // Given
    const InputMap map = InputMap::world_defaults();

    // When
    const auto action = map.action_for(Key::Tab);

    // Then
    EXPECT_EQ(action, Action::ToggleInventory);
}

TEST(InputHandler, WorldDefaultsToggleTheBuildMenuWithB) {
    // Given
    const InputMap map = InputMap::world_defaults();

    // When
    const auto action = map.action_for(Key::B);

    // Then
    EXPECT_EQ(action, Action::ToggleBuildMenu);
}

TEST(InputHandler, WorldDefaultsGetInAndOutOfVehiclesWithF) {
    // Given
    const InputMap map = InputMap::world_defaults();

    // When
    const auto action = map.action_for(Key::F);

    // Then
    EXPECT_EQ(action, Action::ToggleVehicle);
}
