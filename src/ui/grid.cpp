#include "ui/grid.hpp"

#include <algorithm>

namespace ui {

std::vector<Widget*> Grid::shown() const {
    std::vector<Widget*> shown;
    for (Widget* child : children_) {
        if (child->visible()) {
            shown.push_back(child);
        }
    }
    return shown;
}

Size Grid::cell_size(const Canvas& canvas) const {
    Size cell;
    for (const Widget* child : shown()) {
        const Size size = child->measure(canvas);
        cell.width = std::max(cell.width, size.width);
        cell.height = std::max(cell.height, size.height);
    }
    return cell;
}

Size Grid::preferred_size(const Canvas& canvas) const {
    const std::size_t count = shown().size();
    if (count == 0) {
        return resolve_size({});
    }
    const Size cell = cell_size(canvas);
    const std::size_t columns = std::min(count, columns_);
    const std::size_t rows = (count + columns_ - 1) / columns_;
    return resolve_size(
        {static_cast<float>(columns) * cell.width + static_cast<float>(columns - 1) * style_.gap,
         static_cast<float>(rows) * cell.height + static_cast<float>(rows - 1) * style_.gap});
}

void Grid::layout(Rect bounds, const Canvas& canvas) {
    const LayoutPass pass;
    const Size wanted = measure(canvas);
    bounds_ = {bounds.x, bounds.y, std::max(bounds.width, wanted.width),
               std::max(bounds.height, wanted.height)};
    const Size cell = cell_size(canvas);
    std::size_t index = 0;
    for (Widget* child : children_) {
        if (!child->visible()) {
            child->layout({}, canvas); // hidden: no room, no stale bounds
            continue;
        }
        const auto column = static_cast<float>(index % columns_);
        const auto row = static_cast<float>(index / columns_);
        child->layout({bounds_.x + style_.padding.left + column * (cell.width + style_.gap),
                       bounds_.y + style_.padding.top + row * (cell.height + style_.gap),
                       cell.width, cell.height},
                      canvas);
        ++index;
    }
}

void Grid::draw(Canvas& canvas) const {
    draw_box(canvas, style_.background, style_.border_color);
    const std::vector<Widget*> children = shown();
    for (const Widget* child : children) {
        child->draw(canvas);
    }
    for (const Widget* child : children) {
        child->draw_overlay(canvas);
    }
}

void Grid::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    for (Widget* child : shown()) {
        child->handle_pointer(position, released);
    }
}

bool Grid::handle_scroll(Point position, float notches) {
    for (Widget* child : shown()) {
        if (child->handle_scroll(position, notches)) {
            return true;
        }
    }
    return false;
}

} // namespace ui
