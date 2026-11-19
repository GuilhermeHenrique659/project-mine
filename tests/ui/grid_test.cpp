#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "support/fake_canvas.hpp"
#include "ui/grid.hpp"

namespace {

// A 10x10 cell that counts its clicks.
class Cell : public ui::Widget {
public:
    Cell() : ui::Widget(ui::Style{}) {
        set_on_click([this] { ++clicks; });
    }
    ui::Size preferred_size(const ui::Canvas&) const override { return {10.0f, 10.0f}; }
    void draw(ui::Canvas&) const override {}

    int clicks = 0;
};

// Seven cells in a three-column grid with 2-pixel gaps, laid out at (100, 50).
struct Seven {
    Seven() {
        ui::GridBuilder builder = ui::grid(3);
        builder.gap(2.0f);
        for (Cell& cell : cells) {
            builder.child_ref(cell);
        }
        grid = builder.build();
        grid->layout({100.0f, 50.0f, 0.0f, 0.0f}, canvas);
    }

    FakeCanvas canvas;
    Cell cells[7];
    std::unique_ptr<ui::Grid> grid;
};

} // namespace

TEST(Grid, PlacesChildrenLeftToRightThenWrapsToTheNextRow) {
    // Given / When
    Seven g;

    // Then
    EXPECT_FLOAT_EQ(g.cells[2].bounds().x, 124.0f);
    EXPECT_FLOAT_EQ(g.cells[2].bounds().y, 50.0f);
    EXPECT_FLOAT_EQ(g.cells[3].bounds().x, 100.0f);
    EXPECT_FLOAT_EQ(g.cells[3].bounds().y, 62.0f);
    EXPECT_FLOAT_EQ(g.cells[6].bounds().y, 74.0f);
}

TEST(Grid, IsAsWideAsItsColumnsAndAsHighAsItsRows) {
    // Given
    Seven g;

    // When
    const ui::Size size = g.grid->preferred_size(g.canvas);

    // Then: 3 columns, 3 rows of 10 with 2 between
    EXPECT_FLOAT_EQ(size.width, 34.0f);
    EXPECT_FLOAT_EQ(size.height, 34.0f);
}

TEST(Grid, FewerChildrenThanColumnsTakeOnlyTheirWidth) {
    // Given
    FakeCanvas canvas;
    Cell a;
    Cell b;
    auto grid = ui::grid(10).gap(2.0f).child_ref(a).child_ref(b).build();

    // When
    const ui::Size size = grid->preferred_size(canvas);

    // Then
    EXPECT_FLOAT_EQ(size.width, 22.0f);
    EXPECT_FLOAT_EQ(size.height, 10.0f);
}

TEST(Grid, AClickReachesTheCellUnderThePointer) {
    // Given
    Seven g;

    // When
    g.grid->handle_pointer({105.0f, 65.0f}, true);

    // Then
    EXPECT_EQ(g.cells[3].clicks, 1);
    EXPECT_EQ(g.cells[0].clicks, 0);
}
