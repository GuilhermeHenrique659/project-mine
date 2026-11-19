#include <gtest/gtest.h>

#include <memory>

#include "support/fake_canvas.hpp"
#include "ui/box.hpp"

namespace {

// A leaf that always asks for the same size and remembers where it was placed.
class Fixed : public ui::Widget {
public:
    explicit Fixed(ui::Size size) : ui::Widget(ui::Style{}), size_(size) {}
    ui::Size preferred_size(const ui::Canvas&) const override { return size_; }
    void draw(ui::Canvas&) const override {}

private:
    ui::Size size_;
};

void expect_rect(const ui::Rect& actual, ui::Rect expected) {
    EXPECT_FLOAT_EQ(actual.x, expected.x);
    EXPECT_FLOAT_EQ(actual.y, expected.y);
    EXPECT_FLOAT_EQ(actual.width, expected.width);
    EXPECT_FLOAT_EQ(actual.height, expected.height);
}

} // namespace

TEST(Box, AColumnStacksChildrenInsideThePaddingWithTheGapBetweenThem) {
    // Given
    FakeCanvas canvas;
    Fixed a({40.0f, 10.0f});
    Fixed b({20.0f, 30.0f});
    auto box = ui::column().padding(5.0f).gap(4.0f).child_ref(a).child_ref(b).build();

    // When
    box->layout({100.0f, 200.0f, 0.0f, 0.0f}, canvas);

    // Then
    expect_rect(a.bounds(), {105.0f, 205.0f, 40.0f, 10.0f});
    expect_rect(b.bounds(), {105.0f, 219.0f, 20.0f, 30.0f});
}

TEST(Box, PreferredSizeAddsChildrenGapsAndPaddingAlongTheAxisAndTakesTheWidestAcross) {
    // Given
    FakeCanvas canvas;
    Fixed a({40.0f, 10.0f});
    Fixed b({20.0f, 30.0f});
    auto box = ui::column().padding(5.0f).gap(4.0f).child_ref(a).child_ref(b).build();

    // When
    const ui::Size size = box->preferred_size(canvas);

    // Then
    EXPECT_FLOAT_EQ(size.width, 50.0f);         // 40 + 2 * 5
    EXPECT_FLOAT_EQ(size.height, 54.0f);        // 10 + 4 + 30 + 2 * 5
}

TEST(Box, ARowPlacesChildrenSideBySide) {
    // Given
    FakeCanvas canvas;
    Fixed a({40.0f, 10.0f});
    Fixed b({20.0f, 30.0f});
    auto box = ui::row().gap(6.0f).child_ref(a).child_ref(b).build();

    // When
    box->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // Then
    expect_rect(b.bounds(), {46.0f, 0.0f, 20.0f, 30.0f});
    EXPECT_FLOAT_EQ(box->preferred_size(canvas).width, 66.0f);
    EXPECT_FLOAT_EQ(box->preferred_size(canvas).height, 30.0f);
}

TEST(Box, SpaceBetweenPushesTheLastChildToTheFarEnd) {
    // Given: a row given more width than it asks for
    FakeCanvas canvas;
    Fixed title({40.0f, 10.0f});
    Fixed close({10.0f, 10.0f});
    auto box = ui::row().padding(2.0f).justify(ui::Justify::SpaceBetween)
                   .child_ref(title).child_ref(close).build();

    // When
    box->layout({0.0f, 0.0f, 200.0f, 14.0f}, canvas);

    // Then
    EXPECT_FLOAT_EQ(title.bounds().x, 2.0f);
    EXPECT_FLOAT_EQ(close.bounds().x, 188.0f); // 200 - 2 - 10
    EXPECT_FLOAT_EQ(box->bounds().width, 200.0f);
}

TEST(Box, CenterAlignsChildrenInTheMiddleOfTheCrossAxis) {
    // Given
    FakeCanvas canvas;
    Fixed small({10.0f, 10.0f});
    Fixed tall({10.0f, 30.0f});
    auto box = ui::row().align(ui::Align::Center).child_ref(small).child_ref(tall).build();

    // When
    box->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // Then
    EXPECT_FLOAT_EQ(small.bounds().y, 10.0f);
    EXPECT_FLOAT_EQ(tall.bounds().y, 0.0f);
}

TEST(Box, StretchGivesEveryChildTheInnerCrossSize) {
    // Given
    FakeCanvas canvas;
    Fixed narrow({10.0f, 10.0f});
    Fixed wide({60.0f, 10.0f});
    auto box = ui::column().padding(5.0f).align(ui::Align::Stretch)
                   .child_ref(narrow).child_ref(wide).build();

    // When
    box->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // Then
    EXPECT_FLOAT_EQ(narrow.bounds().width, 60.0f);
}

TEST(Box, AnEmptyBoxIsOnlyItsPadding) {
    // Given
    FakeCanvas canvas;
    auto box = ui::column().padding(8.0f).gap(4.0f).build();

    // When
    const ui::Size size = box->preferred_size(canvas);

    // Then
    EXPECT_FLOAT_EQ(size.width, 16.0f);
    EXPECT_FLOAT_EQ(size.height, 16.0f);
}

TEST(Box, AFixedWidthOverridesTheContentWidth) {
    // Given
    FakeCanvas canvas;
    Fixed a({40.0f, 10.0f});
    auto box = ui::column().width(300.0f).align(ui::Align::Stretch).child_ref(a).build();

    // When
    box->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // Then
    EXPECT_FLOAT_EQ(box->bounds().width, 300.0f);
    EXPECT_FLOAT_EQ(a.bounds().width, 300.0f);
}

TEST(Box, CenteredPutsARectInTheMiddleOfTheArea) {
    // Given / When
    const ui::Rect rect = ui::centered({200.0f, 100.0f}, {1280.0f, 720.0f});

    // Then
    expect_rect(rect, {540.0f, 310.0f, 200.0f, 100.0f});
}
