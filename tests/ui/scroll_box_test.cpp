#include <gtest/gtest.h>

#include <memory>

#include "support/fake_canvas.hpp"
#include "ui/box.hpp"
#include "ui/scroll_box.hpp"

namespace {

// A leaf of a fixed size that counts its clicks.
class Fixed : public ui::Widget {
public:
    explicit Fixed(ui::Size size) : ui::Widget(ui::Style{}), size_(size) {
        set_on_click([this] { ++clicks; });
    }
    ui::Size preferred_size(const ui::Canvas&) const override { return size_; }
    void draw(ui::Canvas&) const override {}

    int clicks = 0;

private:
    ui::Size size_;
};

// Ten 20-pixel lines in a 50-pixel high view, scrolled 20 pixels per notch.
struct List {
    List() {
        for (auto& line : lines) {
            column.child_ref(line);
        }
        content = column.build();
        view = ui::scroll_box(*content).step(20.0f).max_height(50.0f).build();
        view->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);
    }

    FakeCanvas canvas;
    Fixed lines[10] = {Fixed({100.0f, 20.0f}), Fixed({100.0f, 20.0f}), Fixed({100.0f, 20.0f}),
                       Fixed({100.0f, 20.0f}), Fixed({100.0f, 20.0f}), Fixed({100.0f, 20.0f}),
                       Fixed({100.0f, 20.0f}), Fixed({100.0f, 20.0f}), Fixed({100.0f, 20.0f}),
                       Fixed({100.0f, 20.0f})};
    ui::BoxBuilder column = ui::column();
    std::unique_ptr<ui::Box> content;
    std::unique_ptr<ui::ScrollBox> view;
};

} // namespace

TEST(ScrollBox, TheViewIsAsHighAsItsMaxHeightWhenTheContentIsTaller) {
    // Given / When
    List list;

    // Then
    EXPECT_FLOAT_EQ(list.view->bounds().height, 50.0f);
    EXPECT_FLOAT_EQ(list.view->max_offset(), 150.0f);
}

TEST(ScrollBox, ScrollingDownOneNotchMovesTheContentUpOneStep) {
    // Given
    List list;

    // When
    list.view->scroll(-1.0f);
    list.view->layout({0.0f, 0.0f, 0.0f, 0.0f}, list.canvas);

    // Then
    EXPECT_FLOAT_EQ(list.lines[1].bounds().y, 0.0f);
}

TEST(ScrollBox, ScrollingStopsAtTheTopAndAtTheEnd) {
    // Given
    List list;

    // When
    list.view->scroll(3.0f);
    const float at_top = list.view->offset();
    list.view->scroll(-100.0f);

    // Then
    EXPECT_FLOAT_EQ(at_top, 0.0f);
    EXPECT_FLOAT_EQ(list.view->offset(), 150.0f);
}

TEST(ScrollBox, ContentThatFitsDoesNotScroll) {
    // Given
    FakeCanvas canvas;
    Fixed line({100.0f, 20.0f});
    auto view = ui::scroll_box(line).step(20.0f).max_height(50.0f).build();
    view->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // When
    view->scroll(-1.0f);

    // Then
    EXPECT_FLOAT_EQ(view->offset(), 0.0f);
    EXPECT_FLOAT_EQ(view->bounds().height, 20.0f);
}

TEST(ScrollBox, ClicksOutsideTheViewNeverReachTheScrolledOutContent) {
    // Given: line 3 sits at y 60..80, below the 50-pixel view
    List list;

    // When
    list.view->handle_pointer({10.0f, 70.0f}, true);

    // Then
    EXPECT_EQ(list.lines[3].clicks, 0);
}

TEST(ScrollBox, ClicksInsideTheViewReachTheContent) {
    // Given
    List list;

    // When
    list.view->handle_pointer({10.0f, 30.0f}, true);

    // Then
    EXPECT_EQ(list.lines[1].clicks, 1);
}

TEST(ScrollBox, TheWheelOverTheViewScrollsItAndIsTaken) {
    // Given
    List list;

    // When
    const bool taken = list.view->handle_scroll({10.0f, 10.0f}, -1.0f);
    const bool outside = list.view->handle_scroll({10.0f, 300.0f}, -1.0f);

    // Then
    EXPECT_TRUE(taken);
    EXPECT_FALSE(outside);
    EXPECT_FLOAT_EQ(list.view->offset(), 20.0f);
}

TEST(ScrollBox, DrawsTheContentClippedToTheView) {
    // Given
    List list;

    // When
    list.view->draw(list.canvas);

    // Then: clipped while drawing, unclipped afterwards
    ASSERT_EQ(list.canvas.clips.size(), 2u);
    ASSERT_TRUE(list.canvas.clips[0]);
    EXPECT_FLOAT_EQ(list.canvas.clips[0]->height, 50.0f);
    EXPECT_FALSE(list.canvas.clips[1]);
}

TEST(ScrollBox, AGrowingViewInAColumnTakesTheRoomLeft) {
    // Given: a 100-high column with a 30-high title above the list
    FakeCanvas canvas;
    Fixed title({100.0f, 30.0f});
    Fixed tall({100.0f, 500.0f});
    auto view = ui::scroll_box(tall).step(20.0f).build();
    ui::ScrollBox& list = *view;
    auto box = ui::column().height(100.0f).child_ref(title).child(std::move(view), {1.0f, 1.0f}).build();

    // When
    box->layout({0.0f, 0.0f, 0.0f, 0.0f}, canvas);

    // Then
    EXPECT_FLOAT_EQ(list.bounds().y, 30.0f);
    EXPECT_FLOAT_EQ(list.bounds().height, 70.0f);
}

TEST(Canvas, NestedClipsIntersect) {
    // Given
    FakeCanvas canvas;
    canvas.push_clip({0.0f, 0.0f, 100.0f, 100.0f});

    // When
    canvas.push_clip({50.0f, 50.0f, 100.0f, 100.0f});

    // Then
    ASSERT_TRUE(canvas.clips.back());
    EXPECT_FLOAT_EQ(canvas.clips.back()->width, 50.0f);
    canvas.pop_clip();
    EXPECT_FLOAT_EQ(canvas.clips.back()->width, 100.0f);
}
