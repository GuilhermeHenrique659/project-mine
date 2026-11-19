#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "ui/widget.hpp"

namespace ui {

// Fixed-column grid, like CSS grid with `grid-template-columns: repeat(N, auto)`: children
// fill the rows left to right, wrapping after `columns`, every cell as big as the biggest
// child, with the style's gap between cells and its padding around them. For inventories,
// recipe pickers and the like:
//   ui::grid(10).gap(6).child(...).child(...).build()
// Hidden children are skipped. Draws its background and border, then the children; the
// pointer and the wheel go to the visible children.
class Grid : public Widget {
public:
    Grid(Style style, std::size_t columns, std::vector<std::unique_ptr<Widget>> owned,
         std::vector<Widget*> children)
        : Widget(std::move(style)), columns_(columns == 0 ? 1 : columns), owned_(std::move(owned)),
          children_(std::move(children)) {}

    Size preferred_size(const Canvas& canvas) const override;
    void layout(Rect bounds, const Canvas& canvas) override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;
    bool handle_scroll(Point position, float notches) override;

    const std::vector<Widget*>& children() const { return children_; }

private:
    std::vector<Widget*> shown() const;
    // The size of one cell: the biggest visible child.
    Size cell_size(const Canvas& canvas) const;

    std::size_t columns_;
    std::vector<std::unique_ptr<Widget>> owned_;
    std::vector<Widget*> children_;
};

class GridBuilder : public HandlerBuilder<GridBuilder> {
public:
    explicit GridBuilder(std::size_t columns) : columns_(columns) {}

    GridBuilder& gap(float value) { style_.gap = value; return *this; }
    GridBuilder& padding(float all) { style_.padding = Insets::all(all); return *this; }
    GridBuilder& background(Color value) { style_.background = value; return *this; }

    // A child the grid owns.
    GridBuilder& child(std::unique_ptr<Widget> widget) {
        children_.push_back(widget.get());
        owned_.push_back(std::move(widget));
        return *this;
    }
    // A child owned by the caller, which must outlive the grid.
    GridBuilder& child_ref(Widget& widget) {
        children_.push_back(&widget);
        return *this;
    }

    // Single use: the children move into the grid.
    std::unique_ptr<Grid> build() {
        auto grid = std::make_unique<Grid>(style_, columns_, std::move(owned_), std::move(children_));
        apply_handlers(*grid);
        return grid;
    }

private:
    Style style_;
    std::size_t columns_;
    std::vector<std::unique_ptr<Widget>> owned_;
    std::vector<Widget*> children_;
};

inline GridBuilder grid(std::size_t columns) { return GridBuilder(columns); }

} // namespace ui
