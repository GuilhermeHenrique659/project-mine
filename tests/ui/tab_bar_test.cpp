#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "support/fake_canvas.hpp"
#include "ui/tab_bar.hpp"
#include "ui/theme.hpp"

namespace {

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

// "Inventory" (enabled, active), "Statistics" (disabled), "Logistics" (enabled), laid out.
struct ThreeTabs {
    ThreeTabs()
        : bar({{"Inventory", true}, {"Statistics", false}, {"Logistics", true}},
              ui::theme::tab_style(), ui::theme::active_tab_style(),
              ui::theme::disabled_tab_style()) {
        bar.set_on_change([this](std::size_t index) { changed_to = index; });
        bar.layout({10.0f, 10.0f, 0.0f, 0.0f}, canvas);
    }

    void click(std::size_t tab) {
        const ui::Point at = center_of(bar.tab_bounds()[tab]);
        bar.handle_pointer(at, false);
        bar.handle_pointer(at, true);
    }

    FakeCanvas canvas;
    ui::TabBar bar;
    std::optional<std::size_t> changed_to;
};

// The colour of the first fill covering exactly this rectangle (a widget's face).
std::optional<ui::Color> face_of(const FakeCanvas& canvas, const ui::Rect& rect) {
    for (const FilledRect& fill : canvas.fills) {
        if (fill.rect.x == rect.x && fill.rect.y == rect.y && fill.rect.width == rect.width &&
            fill.rect.height == rect.height) {
            return fill.color;
        }
    }
    return std::nullopt;
}

std::optional<ui::Color> text_color_of(const FakeCanvas& canvas, const std::string& text) {
    for (const DrawnText& drawn : canvas.texts) {
        if (drawn.text == text) return drawn.color;
    }
    return std::nullopt;
}

} // namespace

TEST(TabBar, ClickingAnEnabledTabMakesItActiveAndReportsTheChange) {
    // Given
    ThreeTabs tabs;

    // When
    tabs.click(2);

    // Then
    EXPECT_EQ(tabs.bar.active(), 2u);
    EXPECT_EQ(tabs.changed_to, 2u);
}

TEST(TabBar, ClickingADisabledTabKeepsTheActiveOne) {
    // Given
    ThreeTabs tabs;

    // When
    tabs.click(1);

    // Then
    EXPECT_EQ(tabs.bar.active(), 0u);
    EXPECT_FALSE(tabs.changed_to.has_value());
}

TEST(TabBar, ClickingTheActiveTabReportsNoChange) {
    // Given
    ThreeTabs tabs;

    // When
    tabs.click(0);

    // Then
    EXPECT_EQ(tabs.bar.active(), 0u);
    EXPECT_FALSE(tabs.changed_to.has_value());
}

TEST(TabBar, TabsSitSideBySideFromLeftToRight) {
    // Given
    ThreeTabs tabs;

    // When
    const auto& bounds = tabs.bar.tab_bounds();

    // Then
    ASSERT_EQ(bounds.size(), 3u);
    EXPECT_FLOAT_EQ(bounds[0].x, 10.0f);
    for (std::size_t i = 1; i < bounds.size(); ++i) {
        EXPECT_GE(bounds[i].x, bounds[i - 1].x + bounds[i - 1].width);
        EXPECT_FLOAT_EQ(bounds[i].y, bounds[0].y);
    }
}

TEST(TabBar, TabsAreDrawnLikeButtonsWithTheActiveOneGreen) {
    // Given
    ThreeTabs tabs;

    // When
    tabs.bar.draw(tabs.canvas);

    // Then
    const auto& bounds = tabs.bar.tab_bounds();
    EXPECT_EQ(face_of(tabs.canvas, bounds[0]), ui::theme::GreenFace);
    EXPECT_EQ(face_of(tabs.canvas, bounds[2]), ui::theme::ButtonFace);
    EXPECT_EQ(text_color_of(tabs.canvas, "Statistics"), ui::theme::Muted);
    EXPECT_EQ(text_color_of(tabs.canvas, "Logistics"), ui::theme::Text);
}

TEST(TabBar, HoveringAnEnabledTabDarkensItLikeAButton) {
    // Given
    ThreeTabs tabs;

    // When
    tabs.bar.handle_pointer(center_of(tabs.bar.tab_bounds()[2]), false);
    tabs.bar.draw(tabs.canvas);

    // Then
    EXPECT_EQ(face_of(tabs.canvas, tabs.bar.tab_bounds()[2]), ui::theme::ButtonFaceHover);
}

TEST(TabBar, HoveringADisabledTabDoesNotChangeIt) {
    // Given
    ThreeTabs tabs;
    tabs.bar.draw(tabs.canvas);
    const auto before = face_of(tabs.canvas, tabs.bar.tab_bounds()[1]);
    FakeCanvas hovered;

    // When
    tabs.bar.handle_pointer(center_of(tabs.bar.tab_bounds()[1]), false);
    tabs.bar.draw(hovered);

    // Then
    EXPECT_EQ(face_of(hovered, tabs.bar.tab_bounds()[1]), before);
}
