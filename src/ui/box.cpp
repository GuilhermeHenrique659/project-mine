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

// A child's own fixed size on the cross axis: it wins over Align::Stretch, as in CSS.
const std::optional<float>& fixed_cross(const Widget& child, Direction direction) {
    return direction == Direction::Row ? child.style().height : child.style().width;
}

// A child's min size on the main axis, the floor of its shrinking.
float min_main(const Widget& child, Direction direction) {
    const auto& min =
        direction == Direction::Row ? child.style().min_width : child.style().min_height;
    return min.value_or(0.0f);
}

} // namespace

std::vector<std::size_t> Box::shown() const {
    std::vector<std::size_t> shown;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (children_[i]->visible()) {
            shown.push_back(i);
        }
    }
    return shown;
}

Size Box::preferred_size(const Canvas& canvas) const {
    const Direction direction = layout_.direction;
    float main = 0.0f;
    float cross = 0.0f;
    const std::vector<std::size_t> shown_children = shown();
    for (const std::size_t i : shown_children) {
        const Size size = children_[i]->measure(canvas);
        main += main_of(size, direction);
        cross = std::max(cross, cross_of(size, direction));
    }
    if (!shown_children.empty()) {
        main += style_.gap * static_cast<float>(shown_children.size() - 1);
    }
    const Size content = direction == Direction::Row ? Size{main, cross} : Size{cross, main};
    return resolve_size(content);
}

void Box::layout(Rect bounds, const Canvas& canvas) {
    const LayoutPass pass;
    // The box takes at least its preferred size and a parent may give it more (stretch,
    // grow), except along a fixed width or height.
    const Size wanted = measure(canvas);
    bounds_ = {bounds.x, bounds.y,
               style_.width ? wanted.width : std::max(bounds.width, wanted.width),
               style_.height ? wanted.height : std::max(bounds.height, wanted.height)};

    const Direction direction = layout_.direction;
    const Insets& padding = style_.padding;
    const bool row = direction == Direction::Row;
    const Rect inner{bounds_.x + padding.left, bounds_.y + padding.top,
                     bounds_.width - padding.horizontal(), bounds_.height - padding.vertical()};
    const float inner_main = row ? inner.width : inner.height;
    const float inner_cross = row ? inner.height : inner.width;

    // Hidden children take no room and keep no bounds, so stale hit-tests miss them.
    for (Widget* child : children_) {
        if (!child->visible()) {
            child->layout({}, canvas);
        }
    }
    const std::vector<std::size_t> shown_children = shown();
    std::vector<float> mains;
    float used = 0.0f;
    float grow = 0.0f;
    float shrink = 0.0f;
    for (const std::size_t i : shown_children) {
        mains.push_back(main_of(children_[i]->measure(canvas), direction));
        used += mains.back();
        grow += flex_[i].grow;
        shrink += flex_[i].shrink;
    }
    const float gaps = shown_children.empty()
                           ? 0.0f
                           : style_.gap * static_cast<float>(shown_children.size() - 1);
    float free = inner_main - used - gaps;
    for (std::size_t n = 0; n < shown_children.size(); ++n) {
        const std::size_t i = shown_children[n];
        if (free > 0.0f && grow > 0.0f) {
            mains[n] += free * flex_[i].grow / grow;
        } else if (free < 0.0f && shrink > 0.0f) {
            mains[n] = std::max(min_main(*children_[i], direction),
                                mains[n] + free * flex_[i].shrink / shrink);
        }
    }
    if (free < 0.0f || grow > 0.0f) {
        free = 0.0f; // the growing children took it, or there was none
    }

    float gap = style_.gap;
    float cursor = row ? inner.x : inner.y;
    switch (layout_.justify) {
    case Justify::Center: cursor += free / 2.0f; break;
    case Justify::End: cursor += free; break;
    case Justify::SpaceBetween:
        if (shown_children.size() > 1) {
            gap += free / static_cast<float>(shown_children.size() - 1);
        }
        break;
    case Justify::Start: break;
    }

    const float cross_start = row ? inner.y : inner.x;
    for (std::size_t n = 0; n < shown_children.size(); ++n) {
        Widget& child = *children_[shown_children[n]];
        const bool stretch = layout_.align == Align::Stretch && !fixed_cross(child, direction);
        const float cross_size = stretch ? inner_cross : cross_of(child.measure(canvas), direction);
        const float cross_pos = cross_start + cross_offset(layout_.align, inner_cross, cross_size);
        child.layout(make_rect(cursor, cross_pos, mains[n], cross_size, direction), canvas);
        cursor += mains[n] + gap;
    }
}

void Box::draw(Canvas& canvas) const {
    draw_box(canvas, style_.background, style_.border_color);
    draw_bevel(canvas);
    const std::vector<std::size_t> shown_children = shown();
    for (const std::size_t i : shown_children) {
        children_[i]->draw(canvas);
    }
    for (const std::size_t i : shown_children) {
        children_[i]->draw_overlay(canvas);
    }
}

bool Box::handle_scroll(Point position, float notches) {
    for (const std::size_t i : shown()) {
        if (children_[i]->handle_scroll(position, notches)) {
            return true;
        }
    }
    return false;
}

void Box::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    const std::vector<std::size_t> shown_children = shown();
    for (const std::size_t i : shown_children) {
        if (children_[i]->blocks_pointer()) {
            children_[i]->handle_pointer(position, released);
            return;
        }
    }
    for (const std::size_t i : shown_children) {
        children_[i]->handle_pointer(position, released);
    }
}

} // namespace ui
