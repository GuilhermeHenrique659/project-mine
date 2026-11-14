#include <gtest/gtest.h>

#include "support/fake_canvas.hpp"
#include "ui/checkbox.hpp"

namespace {

constexpr ui::Rect Bounds{0.0f, 0.0f, 200.0f, 30.0f};
constexpr ui::Point Inside{10.0f, 10.0f};
constexpr ui::Point Outside{500.0f, 500.0f};

} // namespace

TEST(Checkbox, ClickingTogglesAndNotifiesTheNewValue) {
    // Given
    FakeCanvas canvas;
    int notifications = 0;
    bool last_value = false;
    auto checkbox = ui::checkbox("Fullscreen")
                        .on_change([&](bool value) {
                            ++notifications;
                            last_value = value;
                        })
                        .build();
    checkbox->layout(Bounds, canvas);

    // When
    checkbox->handle_pointer(Inside, true);

    // Then
    EXPECT_TRUE(checkbox->checked());
    EXPECT_EQ(notifications, 1);
    EXPECT_TRUE(last_value);
}

TEST(Checkbox, ClickingTwiceUnchecksIt) {
    // Given
    FakeCanvas canvas;
    auto checkbox = ui::checkbox("Fullscreen").build();
    checkbox->layout(Bounds, canvas);

    // When
    checkbox->handle_pointer(Inside, true);
    checkbox->handle_pointer(Inside, true);

    // Then
    EXPECT_FALSE(checkbox->checked());
}

TEST(Checkbox, ReleasingOutsideDoesNotToggle) {
    // Given
    FakeCanvas canvas;
    auto checkbox = ui::checkbox("Fullscreen").build();
    checkbox->layout(Bounds, canvas);

    // When
    checkbox->handle_pointer(Outside, true);

    // Then
    EXPECT_FALSE(checkbox->checked());
}

TEST(Checkbox, KeyboardActivationToggles) {
    // Given
    auto checkbox = ui::checkbox("Fullscreen").checked(true).build();

    // When
    checkbox->activate();

    // Then
    EXPECT_FALSE(checkbox->checked());
}

TEST(Checkbox, CheckedBoxDrawsAMarkThatUncheckedDoesNot) {
    // Given
    FakeCanvas checked_canvas;
    FakeCanvas unchecked_canvas;
    auto checked = ui::checkbox("Fullscreen").checked(true).build();
    auto unchecked = ui::checkbox("Fullscreen").checked(false).build();
    checked->layout(Bounds, checked_canvas);
    unchecked->layout(Bounds, unchecked_canvas);

    // When
    checked->draw(checked_canvas);
    unchecked->draw(unchecked_canvas);

    // Then
    EXPECT_EQ(checked_canvas.fills.size(), unchecked_canvas.fills.size() + 1);
}
