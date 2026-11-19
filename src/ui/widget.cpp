#include "ui/widget.hpp"

namespace ui {

void Widget::layout(Rect bounds, const Canvas&) { bounds_ = bounds; }

Size Widget::measure(const Canvas& canvas) const {
    if (!LayoutPass::active()) {
        return preferred_size(canvas);
    }
    if (measured_pass_ != LayoutPass::generation()) {
        measured_ = preferred_size(canvas);
        measured_pass_ = LayoutPass::generation();
    }
    return measured_;
}

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

namespace {
float clamp_to(float value, const std::optional<float>& min, const std::optional<float>& max) {
    if (max && value > *max) {
        value = *max;
    }
    if (min && value < *min) {
        value = *min;
    }
    return value;
}
} // namespace

Size Widget::resolve_size(Size content) const {
    return {clamp_to(style_.width.value_or(content.width + style_.padding.horizontal()),
                     style_.min_width, style_.max_width),
            clamp_to(style_.height.value_or(content.height + style_.padding.vertical()),
                     style_.min_height, style_.max_height)};
}

void Widget::draw_box(Canvas& canvas, Color background, Color border) const {
    if (background.a > 0) {
        canvas.fill_rect(bounds_, background);
    }
    if (style_.border_width > 0.0f && border.a > 0) {
        canvas.stroke_rect(bounds_, border, style_.border_width);
    }
}

// Dark band on the bottom and right edges, inside the border.
void Widget::draw_bevel(Canvas& canvas, Rect area) const {
    if (!style_.bevel) {
        return;
    }
    const float inset = style_.border_width;
    const float width = style_.bevel->width;
    const Rect r{area.x + inset, area.y + inset, area.width - 2.0f * inset,
                 area.height - 2.0f * inset};
    canvas.fill_rect({r.x, r.y + r.height - width, r.width, width}, style_.bevel->shadow);
    canvas.fill_rect({r.x + r.width - width, r.y, width, r.height - width}, style_.bevel->shadow);
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

void draw_raised(Canvas& canvas, Rect area, Color face, const Style& style) {
    canvas.fill_rect(area, face);
    if (style.border_width > 0.0f && style.border_color.a > 0) {
        canvas.stroke_rect(area, style.border_color, style.border_width);
    }
    if (!style.bevel) {
        return;
    }
    const float inset = style.border_width;
    const float width = style.bevel->width;
    const Rect r{area.x + inset, area.y + inset, area.width - 2.0f * inset,
                 area.height - 2.0f * inset};
    canvas.fill_rect({r.x, r.y + r.height - width, r.width, width}, style.bevel->shadow);
    canvas.fill_rect({r.x + r.width - width, r.y, width, r.height - width}, style.bevel->shadow);
}

} // namespace ui
