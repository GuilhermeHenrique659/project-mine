#include <gtest/gtest.h>

#include <optional>

#include "support/fake_canvas.hpp"
#include "ui/button.hpp"

TEST(Button, AnIconIsDrawnAsASquareOnTheLeftOfTheBox) {
    // Given
    FakeCanvas canvas;
    std::optional<ui::Rect> icon_rect;
    auto button = ui::button("Play").build();
    button->set_icon([&](ui::Canvas&, ui::Rect rect) { icon_rect = rect; });
    button->layout({100.0f, 50.0f, 300.0f, 40.0f}, canvas);

    // When
    button->draw(canvas);

    // Then
    ASSERT_TRUE(icon_rect.has_value());
    EXPECT_FLOAT_EQ(icon_rect->width, icon_rect->height);
    EXPECT_GT(icon_rect->x, 100.0f);
    EXPECT_LT(icon_rect->x + icon_rect->width, 100.0f + 300.0f / 2.0f);
    EXPECT_GE(icon_rect->y, 50.0f);
    EXPECT_LE(icon_rect->y + icon_rect->height, 90.0f);
}
