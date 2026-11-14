#include "ui/card.hpp"

#include <algorithm>

namespace ui {

Size Card::preferred_size(const Canvas& canvas) const {
    Size content;
    for (const auto& child : children_) {
        const Size child_size = child->preferred_size(canvas);
        content.width = std::max(content.width, child_size.width);
        content.height += child_size.height;
    }
    if (!children_.empty()) {
        content.height += style_.gap * static_cast<float>(children_.size() - 1);
    }
    return resolve_size(content);
}

void Card::layout(Rect bounds, const Canvas& canvas) {
    const Size size = preferred_size(canvas);
    bounds_ = {bounds.x, bounds.y, size.width, size.height};

    const float inner_x = bounds_.x + style_.padding.left;
    const float inner_width = size.width - style_.padding.horizontal();
    float cursor_y = bounds_.y + style_.padding.top;
    for (const auto& child : children_) {
        const float child_height = child->preferred_size(canvas).height;
        child->layout({inner_x, cursor_y, inner_width, child_height}, canvas);
        cursor_y += child_height + style_.gap;
    }
}

void Card::draw(Canvas& canvas) const {
    draw_box(canvas, style_.background, style_.border_color);
    for (const auto& child : children_) {
        child->draw(canvas);
    }
    for (const auto& child : children_) {
        child->draw_overlay(canvas);
    }
}

void Card::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    for (const auto& child : children_) {
        if (child->blocks_pointer()) {
            child->handle_pointer(position, released);
            return;
        }
    }
    for (const auto& child : children_) {
        child->handle_pointer(position, released);
    }
}

} // namespace ui
