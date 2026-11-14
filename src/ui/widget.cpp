#include "ui/widget.hpp"

namespace ui {

void Widget::layout(Rect bounds, const Canvas&) { bounds_ = bounds; }

void Widget::click() {
    if (on_click_) on_click_();
}

void Widget::handle_pointer(Point position, bool released) {
    const bool inside = bounds_.contains(position);

    if (inside != hovered_) {
        hovered_ = inside;
        const EventHandler& transition = inside ? on_hover_ : on_leave_;
        if (transition) transition();
    }

    if (released && inside && on_click_) {
        on_click_();
    }
}

Size Widget::resolve_size(Size content) const {
    return {style_.width.value_or(content.width + style_.padding.horizontal()),
            style_.height.value_or(content.height + style_.padding.vertical())};
}

void Widget::draw_box(Canvas& canvas, Color background, Color border) const {
    if (background.a > 0) {
        canvas.fill_rect(bounds_, background);
    }
    if (style_.border_width > 0.0f && border.a > 0) {
        canvas.stroke_rect(bounds_, border, style_.border_width);
    }
}

void Widget::draw_text_in_box(Canvas& canvas, const std::string& text, Color color) const {
    const Size measured = canvas.measure_text(text, style_.font_size);
    float x = bounds_.x + style_.padding.left;
    if (style_.text_align == TextAlign::Center) {
        x = bounds_.x + (bounds_.width - measured.width) / 2.0f;
    }
    const float y = bounds_.y + (bounds_.height - measured.height) / 2.0f;
    canvas.draw_text(text, {x, y}, style_.font_size, color);
}

} // namespace ui
