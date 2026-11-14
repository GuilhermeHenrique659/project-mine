#include <gtest/gtest.h>

#include <cstddef>

#include "support/fake_canvas.hpp"
#include "ui/dropdown.hpp"

namespace {

constexpr ui::Rect Bounds{0.0f, 0.0f, 200.0f, 40.0f};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

std::unique_ptr<ui::Dropdown> make_dropdown(std::size_t selected = 0,
                                            ui::Dropdown::ChangeHandler on_change = {}) {
    return ui::dropdown()
        .options({"A", "B", "C"})
        .selected(selected)
        .on_change(std::move(on_change))
        .build();
}

} // namespace

TEST(Dropdown, ClickingTheHeaderOpensTheList) {
    // Given
    FakeCanvas canvas;
    auto dropdown = make_dropdown();
    dropdown->layout(Bounds, canvas);

    // When
    dropdown->handle_pointer(center_of(Bounds), true);

    // Then
    EXPECT_TRUE(dropdown->is_open());
}

TEST(Dropdown, ClickingAnOptionSelectsItNotifiesAndCloses) {
    // Given
    FakeCanvas canvas;
    std::size_t notified = 99;
    auto dropdown = make_dropdown(0, [&](std::size_t index) { notified = index; });
    dropdown->layout(Bounds, canvas);
    dropdown->handle_pointer(center_of(Bounds), true);

    // When
    dropdown->handle_pointer(center_of(dropdown->option_bounds(2)), true);

    // Then
    EXPECT_EQ(dropdown->selected(), 2u);
    EXPECT_EQ(notified, 2u);
    EXPECT_FALSE(dropdown->is_open());
}

TEST(Dropdown, ClickingOutsideClosesWithoutChangingTheSelection) {
    // Given
    FakeCanvas canvas;
    int notifications = 0;
    auto dropdown = make_dropdown(1, [&](std::size_t) { ++notifications; });
    dropdown->layout(Bounds, canvas);
    dropdown->handle_pointer(center_of(Bounds), true);

    // When
    dropdown->handle_pointer({900.0f, 900.0f}, true);

    // Then
    EXPECT_FALSE(dropdown->is_open());
    EXPECT_EQ(dropdown->selected(), 1u);
    EXPECT_EQ(notifications, 0);
}

TEST(Dropdown, ChoosingTheCurrentOptionDoesNotNotify) {
    // Given
    FakeCanvas canvas;
    int notifications = 0;
    auto dropdown = make_dropdown(1, [&](std::size_t) { ++notifications; });
    dropdown->layout(Bounds, canvas);
    dropdown->handle_pointer(center_of(Bounds), true);

    // When
    dropdown->handle_pointer(center_of(dropdown->option_bounds(1)), true);

    // Then
    EXPECT_EQ(notifications, 0);
}

TEST(Dropdown, KeyboardActivationCyclesThroughOptionsAndWraps) {
    // Given
    auto dropdown = make_dropdown(1);

    // When
    dropdown->activate();
    dropdown->activate();

    // Then
    EXPECT_EQ(dropdown->selected(), 0u);
}

TEST(Dropdown, OptionsAreDrawnOnlyWhileOpen) {
    // Given
    FakeCanvas closed_canvas;
    FakeCanvas open_canvas;
    auto closed = make_dropdown();
    auto open = make_dropdown();
    closed->layout(Bounds, closed_canvas);
    open->layout(Bounds, open_canvas);
    open->handle_pointer(center_of(Bounds), true);

    // When
    closed->draw_overlay(closed_canvas);
    open->draw_overlay(open_canvas);

    // Then
    EXPECT_TRUE(closed_canvas.texts.empty());
    ASSERT_EQ(open_canvas.texts.size(), 3u);
    EXPECT_EQ(open_canvas.texts[2].text, "C");
}

TEST(Dropdown, PointerOverAnOpenOptionHighlightsIt) {
    // Given
    FakeCanvas canvas;
    auto dropdown = make_dropdown(0);
    dropdown->layout(Bounds, canvas);
    dropdown->handle_pointer(center_of(Bounds), true);

    // When
    dropdown->handle_pointer(center_of(dropdown->option_bounds(2)), false);
    dropdown->draw_overlay(canvas);

    // Then
    EXPECT_EQ(dropdown->hovered_option(), 2u);
    ASSERT_GE(canvas.fills.size(), 3u);
    EXPECT_EQ(canvas.fills[2].color, dropdown->style().hover_background.value());
    EXPECT_NE(canvas.fills[0].color, dropdown->style().hover_background.value());
}

TEST(Dropdown, MovingOffTheOptionsClearsTheHighlight) {
    // Given
    FakeCanvas canvas;
    auto dropdown = make_dropdown(0);
    dropdown->layout(Bounds, canvas);
    dropdown->handle_pointer(center_of(Bounds), true);
    dropdown->handle_pointer(center_of(dropdown->option_bounds(1)), false);

    // When
    dropdown->handle_pointer({900.0f, 900.0f}, false);

    // Then
    EXPECT_FALSE(dropdown->hovered_option().has_value());
}
