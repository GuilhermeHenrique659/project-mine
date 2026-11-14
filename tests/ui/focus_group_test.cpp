#include <gtest/gtest.h>

#include "ui/focus_group.hpp"

namespace {

struct FakeFocusable : ui::Focusable {
    void set_focused(bool value) override { focused = value; }
    void activate() override { ++activations; }

    bool focused = false;
    int activations = 0;
};

} // namespace

TEST(UiFocusGroup, FirstAddedItemStartsFocused) {
    // Given
    ui::FocusGroup group;
    FakeFocusable first;
    FakeFocusable second;

    // When
    group.add(first);
    group.add(second);

    // Then
    EXPECT_TRUE(first.focused);
    EXPECT_FALSE(second.focused);
}

TEST(UiFocusGroup, FocusNextMovesFocusToTheFollowingItem) {
    // Given
    ui::FocusGroup group;
    FakeFocusable first;
    FakeFocusable second;
    group.add(first);
    group.add(second);

    // When
    group.focus_next();

    // Then
    EXPECT_FALSE(first.focused);
    EXPECT_TRUE(second.focused);
}

TEST(UiFocusGroup, FocusNextWrapsFromLastToFirst) {
    // Given
    ui::FocusGroup group;
    FakeFocusable first;
    FakeFocusable second;
    group.add(first);
    group.add(second);
    group.focus_next();

    // When
    group.focus_next();

    // Then
    EXPECT_TRUE(first.focused);
    EXPECT_FALSE(second.focused);
}

TEST(UiFocusGroup, FocusPreviousWrapsFromFirstToLast) {
    // Given
    ui::FocusGroup group;
    FakeFocusable first;
    FakeFocusable second;
    group.add(first);
    group.add(second);

    // When
    group.focus_previous();

    // Then
    EXPECT_FALSE(first.focused);
    EXPECT_TRUE(second.focused);
}

TEST(UiFocusGroup, ActivateFocusedActivatesOnlyTheFocusedItem) {
    // Given
    ui::FocusGroup group;
    FakeFocusable first;
    FakeFocusable second;
    group.add(first);
    group.add(second);
    group.focus_next();

    // When
    group.activate_focused();

    // Then
    EXPECT_EQ(first.activations, 0);
    EXPECT_EQ(second.activations, 1);
}

TEST(UiFocusGroup, FocusingAnItemJumpsToIt) {
    // Given
    ui::FocusGroup group;
    FakeFocusable first;
    FakeFocusable second;
    FakeFocusable third;
    group.add(first);
    group.add(second);
    group.add(third);

    // When
    group.focus(third);

    // Then
    EXPECT_FALSE(first.focused);
    EXPECT_TRUE(third.focused);
}

TEST(UiFocusGroup, FocusingAnItemOutsideTheGroupIsIgnored) {
    // Given
    ui::FocusGroup group;
    FakeFocusable first;
    FakeFocusable stranger;
    group.add(first);

    // When
    group.focus(stranger);

    // Then
    EXPECT_TRUE(first.focused);
    EXPECT_FALSE(stranger.focused);
}

TEST(UiFocusGroup, EmptyGroupIgnoresNavigationAndActivation) {
    // Given
    ui::FocusGroup group;

    // When / Then
    group.focus_next();
    group.focus_previous();
    group.activate_focused();
    SUCCEED();
}
