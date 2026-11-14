#include <gtest/gtest.h>

#include <string>

#include "support/fake_canvas.hpp"
#include "ui/ui.hpp"

namespace {

constexpr ui::Color Red{255, 0, 0, 255};
constexpr ui::Color Blue{0, 0, 255, 255};

ui::Style plain_text_style() { return ui::Style{}.with_font_size(20); }

} // namespace

TEST(UiCard, StacksChildrenVerticallyWithPaddingAndGap) {
    // Given
    FakeCanvas canvas;
    auto card = ui::card()
                    .style(ui::Style{}
                               .with_padding(ui::Insets::all(10.0f))
                               .with_gap(5.0f)
                               .with_width(200.0f))
                    .child(ui::label("a").style(plain_text_style()).build())
                    .child(ui::label("b").style(plain_text_style()).build())
                    .build();

    // When
    card->layout({0.0f, 0.0f, 500.0f, 500.0f}, canvas);

    // Then
    EXPECT_FLOAT_EQ(card->children()[0]->bounds().y, 10.0f);
    EXPECT_FLOAT_EQ(card->children()[1]->bounds().y, 35.0f);
    EXPECT_FLOAT_EQ(card->children()[0]->bounds().width, 180.0f);
    EXPECT_FLOAT_EQ(card->bounds().height, 65.0f);
}

TEST(UiCard, WithoutWidthFitsTheWidestChild) {
    // Given
    FakeCanvas canvas;
    auto card = ui::card()
                    .style(ui::Style{}.with_padding(ui::Insets::all(10.0f)))
                    .child(ui::label("ab").style(plain_text_style()).build())
                    .child(ui::label("abcd").style(plain_text_style()).build())
                    .build();

    // When
    card->layout({0.0f, 0.0f, 500.0f, 500.0f}, canvas);

    // Then
    EXPECT_FLOAT_EQ(card->bounds().width, 60.0f);
}

TEST(UiLabel, CenteredTextIsDrawnInTheMiddleOfItsBox) {
    // Given
    FakeCanvas canvas;
    auto label = ui::label("ab")
                     .style(plain_text_style().with_width(100.0f).with_text_align(ui::TextAlign::Center))
                     .build();
    label->layout({0.0f, 0.0f, 100.0f, 20.0f}, canvas);

    // When
    label->draw(canvas);

    // Then
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_FLOAT_EQ(canvas.texts[0].position.x, 40.0f);
}

TEST(UiButton, ReleasingInsideTriggersClick) {
    // Given
    FakeCanvas canvas;
    int clicks = 0;
    auto button = ui::button("Play").on_click([&] { ++clicks; }).build();
    button->layout({0.0f, 0.0f, 100.0f, 40.0f}, canvas);

    // When
    button->handle_pointer({50.0f, 20.0f}, true);

    // Then
    EXPECT_EQ(clicks, 1);
}

TEST(UiButton, ReleasingOutsideDoesNotTriggerClick) {
    // Given
    FakeCanvas canvas;
    int clicks = 0;
    auto button = ui::button("Play").on_click([&] { ++clicks; }).build();
    button->layout({0.0f, 0.0f, 100.0f, 40.0f}, canvas);

    // When
    button->handle_pointer({150.0f, 20.0f}, true);

    // Then
    EXPECT_EQ(clicks, 0);
}

TEST(UiButton, DisabledButtonIgnoresClicks) {
    // Given
    FakeCanvas canvas;
    int clicks = 0;
    auto button = ui::button("Play").on_click([&] { ++clicks; }).enabled(false).build();
    button->layout({0.0f, 0.0f, 100.0f, 40.0f}, canvas);

    // When
    button->handle_pointer({50.0f, 20.0f}, true);

    // Then
    EXPECT_EQ(clicks, 0);
}

TEST(UiButton, HoveredButtonDrawsHoverBackground) {
    // Given
    FakeCanvas canvas;
    auto button = ui::button("Play")
                      .style(ui::Style{}.with_background(Blue).with_hover_background(Red))
                      .build();
    button->layout({0.0f, 0.0f, 100.0f, 40.0f}, canvas);
    button->handle_pointer({5.0f, 5.0f}, false);

    // When
    button->draw(canvas);

    // Then
    ASSERT_FALSE(canvas.fills.empty());
    EXPECT_EQ(canvas.fills.front().color, Red);
}

TEST(UiButton, BeveledButtonDrawsShadowOnlyOnBottomAndRightEdges) {
    // Given
    FakeCanvas canvas;
    auto button = ui::button("Play")
                      .style(ui::Style{}.with_background(Blue).with_bevel(Red, 4.0f))
                      .build();
    button->layout({10.0f, 20.0f, 100.0f, 40.0f}, canvas);

    // When
    button->draw(canvas);

    // Then
    ASSERT_EQ(canvas.fills.size(), 3u);
    EXPECT_EQ(canvas.fills[0].color, Blue);
    const FilledRect& bottom = canvas.fills[1];
    EXPECT_EQ(bottom.color, Red);
    EXPECT_FLOAT_EQ(bottom.rect.y, 56.0f);
    EXPECT_FLOAT_EQ(bottom.rect.width, 100.0f);
    EXPECT_FLOAT_EQ(bottom.rect.height, 4.0f);
    const FilledRect& right = canvas.fills[2];
    EXPECT_EQ(right.color, Red);
    EXPECT_FLOAT_EQ(right.rect.x, 106.0f);
    EXPECT_FLOAT_EQ(right.rect.width, 4.0f);
    EXPECT_FLOAT_EQ(right.rect.height, 36.0f);
}

TEST(UiCard, BeveledCardDrawsShadowOnBottomAndRightEdges) {
    // Given
    FakeCanvas canvas;
    auto card = ui::card()
                    .style(ui::Style{}.with_background(Blue).with_bevel(Red, 6.0f).with_width(200.0f).with_height(100.0f))
                    .build();
    card->layout({0.0f, 0.0f, 500.0f, 500.0f}, canvas);

    // When
    card->draw(canvas);

    // Then
    ASSERT_EQ(canvas.fills.size(), 3u);
    EXPECT_FLOAT_EQ(canvas.fills[1].rect.y, 94.0f);
    EXPECT_FLOAT_EQ(canvas.fills[2].rect.x, 194.0f);
    EXPECT_FLOAT_EQ(canvas.fills[2].rect.width, 6.0f);
}

TEST(UiInput, TypedCharactersAreAppendedWhenFocused) {
    // Given
    auto input = ui::input().build();
    input->set_focused(true);

    // When
    input->type_char('a');
    input->type_char('b');

    // Then
    EXPECT_EQ(input->value(), "ab");
}

TEST(UiInput, TypedCharactersAreIgnoredWhenNotFocused) {
    // Given
    auto input = ui::input().build();

    // When
    input->type_char('a');

    // Then
    EXPECT_EQ(input->value(), "");
}

TEST(UiInput, StopsAcceptingCharactersAtMaxLength) {
    // Given
    auto input = ui::input().max_length(3).build();
    input->set_focused(true);

    // When
    for (const char character : std::string("abcdef")) input->type_char(character);

    // Then
    EXPECT_EQ(input->value(), "abc");
}

TEST(UiInput, ErasingRemovesLastCharacterAndNotifiesListener) {
    // Given
    std::string notified;
    auto input = ui::input().value("abc").on_change([&](const std::string& v) { notified = v; }).build();
    input->set_focused(true);

    // When
    input->erase_last();

    // Then
    EXPECT_EQ(input->value(), "ab");
    EXPECT_EQ(notified, "ab");
}

TEST(UiInput, ClickingFocusesAndClickingElsewhereBlurs) {
    // Given
    FakeCanvas canvas;
    auto input = ui::input().build();
    input->layout({0.0f, 0.0f, 200.0f, 40.0f}, canvas);

    // When
    input->handle_pointer({10.0f, 10.0f}, true);
    const bool focused_after_click = input->focused();
    input->handle_pointer({500.0f, 500.0f}, true);

    // Then
    EXPECT_TRUE(focused_after_click);
    EXPECT_FALSE(input->focused());
}

TEST(UiEvents, HoverHandlerFiresOnceWhenPointerEnters) {
    // Given
    FakeCanvas canvas;
    int hovers = 0;
    auto button = ui::button("Play").on_hover([&] { ++hovers; }).build();
    button->layout({0.0f, 0.0f, 100.0f, 40.0f}, canvas);

    // When
    button->handle_pointer({10.0f, 10.0f}, false);
    button->handle_pointer({20.0f, 10.0f}, false);

    // Then
    EXPECT_EQ(hovers, 1);
}

TEST(UiEvents, LeaveHandlerFiresWhenPointerExits) {
    // Given
    FakeCanvas canvas;
    int leaves = 0;
    auto button = ui::button("Play").on_leave([&] { ++leaves; }).build();
    button->layout({0.0f, 0.0f, 100.0f, 40.0f}, canvas);
    button->handle_pointer({10.0f, 10.0f}, false);

    // When
    button->handle_pointer({300.0f, 300.0f}, false);

    // Then
    EXPECT_EQ(leaves, 1);
}

TEST(UiEvents, ClickHandlerWorksOnAnyWidget) {
    // Given
    FakeCanvas canvas;
    int clicks = 0;
    auto label = ui::label("link").on_click([&] { ++clicks; }).build();
    label->layout({0.0f, 0.0f, 100.0f, 20.0f}, canvas);

    // When
    label->handle_pointer({10.0f, 10.0f}, true);

    // Then
    EXPECT_EQ(clicks, 1);
}

TEST(UiEvents, CardForwardsPointerEventsToChildren) {
    // Given
    FakeCanvas canvas;
    int clicks = 0;
    auto card = ui::card()
                    .style(ui::Style{}.with_width(200.0f))
                    .child(ui::button("Play").on_click([&] { ++clicks; }).build())
                    .build();
    card->layout({0.0f, 0.0f, 200.0f, 200.0f}, canvas);

    // When
    card->handle_pointer({10.0f, 10.0f}, true);

    // Then
    EXPECT_EQ(clicks, 1);
}

TEST(UiEvents, DisabledButtonIgnoresProgrammaticClick) {
    // Given
    int clicks = 0;
    auto button = ui::button("Play").on_click([&] { ++clicks; }).enabled(false).build();

    // When
    button->click();

    // Then
    EXPECT_EQ(clicks, 0);
}
