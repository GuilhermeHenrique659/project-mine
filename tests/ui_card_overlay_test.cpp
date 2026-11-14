#include <gtest/gtest.h>

#include "fake_canvas.hpp"
#include "ui/card.hpp"
#include "ui/checkbox.hpp"
#include "ui/dropdown.hpp"

namespace {

constexpr ui::Rect CardBounds{0.0f, 0.0f, 300.0f, 0.0f};

struct Fixture {
    Fixture() {
        auto dropdown_widget = ui::dropdown().options({"A", "B"}).build();
        auto checkbox_widget = ui::checkbox("Below").build();
        dropdown = dropdown_widget.get();
        checkbox = checkbox_widget.get();
        card = ui::card()
                   .style(ui::Style{}.with_gap(0.0f))
                   .child(std::move(dropdown_widget))
                   .child(std::move(checkbox_widget))
                   .build();
        card->layout(CardBounds, canvas);
    }

    FakeCanvas canvas;
    ui::Dropdown* dropdown = nullptr;
    ui::Checkbox* checkbox = nullptr;
    std::unique_ptr<ui::Card> card;
};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(CardOverlay, ClickingAnOpenDropdownOptionDoesNotReachTheWidgetUnderneath) {
    // Given
    Fixture f;
    f.card->handle_pointer(center_of(f.dropdown->bounds()), true);
    // The first option overlaps the checkbox that sits right below the header.
    const ui::Point option_center = center_of(f.dropdown->option_bounds(0));
    ASSERT_TRUE(f.checkbox->bounds().contains(option_center));

    // When
    f.card->handle_pointer(option_center, true);

    // Then
    EXPECT_FALSE(f.checkbox->checked());
    EXPECT_FALSE(f.dropdown->is_open());
}

TEST(CardOverlay, OpenDropdownListIsDrawnAfterItsSiblings) {
    // Given
    Fixture f;
    f.card->handle_pointer(center_of(f.dropdown->bounds()), true);

    // When
    f.card->draw(f.canvas);

    // Then
    ASSERT_FALSE(f.canvas.texts.empty());
    EXPECT_EQ(f.canvas.texts.back().text, "B");
}
