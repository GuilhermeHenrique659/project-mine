#include "ui/box.hpp"

#include <algorithm>

namespace ui {

namespace {

// Sizes and positions read along the box's main axis ("main") and the other one ("cross"),
// so one algorithm serves rows and columns.
float main_of(Size size, Direction direction) {
    return direction == Direction::Row ? size.width : size.height;
}

float cross_of(Size size, Direction direction) {
    return direction == Direction::Row ? size.height : size.width;
}

Rect make_rect(float main_pos, float cross_pos, float main_size, float cross_size,
               Direction direction) {
    if (direction == Direction::Row) {
        return {main_pos, cross_pos, main_size, cross_size};
    }
    return {cross_pos, main_pos, cross_size, main_size};
}

// Offset of a child of `size` inside `room` on the cross axis.
float cross_offset(Align align, float room, float size) {
    switch (align) {
    case Align::Center: return (room - size) / 2.0f;
    case Align::End: return room - size;
    case Align::Start:
    case Align::Stretch: break;
    }
    return 0.0f;
}

} // namespace

Size Box::preferred_size(const Canvas& canvas) const {
    const Direction direction = layout_.direction;
    float main = 0.0f;
    float cross = 0.0f;
    for (const Widget* child : children_) {
        const Size size = child->preferred_size(canvas);
        main += main_of(size, direction);
        cross = std::max(cross, cross_of(size, direction));
    }
    if (!children_.empty()) {
        main += style_.gap * static_cast<float>(children_.size() - 1);
    }
    const Size content = direction == Direction::Row ? Size{main, cross} : Size{cross, main};
    return resolve_size(content);
}

void Box::layout(Rect bounds, const Canvas& canvas) {
    // The box takes at least its preferred size; a parent may give it more (stretch).
    const Size wanted = preferred_size(canvas);
    bounds_ = {bounds.x, bounds.y, std::max(bounds.width, wanted.width),
               std::max(bounds.height, wanted.height)};

    const Direction direction = layout_.direction;
    const Insets& padding = style_.padding;
    const bool row = direction == Direction::Row;
    const Rect inner{bounds_.x + padding.left, bounds_.y + padding.top,
                     bounds_.width - padding.horizontal(), bounds_.height - padding.vertical()};
    const float inner_main = row ? inner.width : inner.height;
    const float inner_cross = row ? inner.height : inner.width;

    std::vector<Size> sizes;
    float used = 0.0f;
    for (const Widget* child : children_) {
        sizes.push_back(child->preferred_size(canvas));
        used += main_of(sizes.back(), direction);
    }
    const float gaps = children_.empty() ? 0.0f : style_.gap * static_cast<float>(children_.size() - 1);
    const float free = std::max(0.0f, inner_main - used - gaps);

    float gap = style_.gap;
    float cursor = row ? inner.x : inner.y;
    switch (layout_.justify) {
    case Justify::Center: cursor += free / 2.0f; break;
    case Justify::End: cursor += free; break;
    case Justify::SpaceBetween:
        if (children_.size() > 1) {
            gap += free / static_cast<float>(children_.size() - 1);
        }
        break;
    case Justify::Start: break;
    }

    const float cross_start = row ? inner.y : inner.x;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        const float main_size = main_of(sizes[i], direction);
        const float cross_size =
            layout_.align == Align::Stretch ? inner_cross : cross_of(sizes[i], direction);
        const float cross_pos = cross_start + cross_offset(layout_.align, inner_cross, cross_size);
        children_[i]->layout(make_rect(cursor, cross_pos, main_size, cross_size, direction), canvas);
        cursor += main_size + gap;
    }
}

void Box::draw(Canvas& canvas) const {
    draw_box(canvas, style_.background, style_.border_color);
    for (const Widget* child : children_) {
        child->draw(canvas);
    }
    for (const Widget* child : children_) {
        child->draw_overlay(canvas);
    }
}

void Box::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    for (Widget* child : children_) {
        if (child->blocks_pointer()) {
            child->handle_pointer(position, released);
            return;
        }
    }
    for (Widget* child : children_) {
        child->handle_pointer(position, released);
    }
}

} // namespace ui
