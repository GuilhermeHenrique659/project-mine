#include "ui/checkbox.hpp"

#include <algorithm>

namespace ui {

namespace {
constexpr float LabelGap = 12.0f;
constexpr float MarkInset = 4.0f;
} // namespace

Size Checkbox::preferred_size(const Canvas& canvas) const {
    const float box = static_cast<float>(style_.font_size);
    const Size text = canvas.measure_text(label_, style_.font_size);
    return resolve_size({box + LabelGap + text.width, std::max(box, text.height)});
}

void Checkbox::draw(Canvas& canvas) const {
    const Color border = focused_ || hovered()
                             ? style_.focus_border_color.value_or(style_.border_color)
                             : style_.border_color;
    draw_box(canvas, style_.background, border);

    const float side = static_cast<float>(style_.font_size);
    const float box_x = bounds_.x + style_.padding.left;
    const float box_y = bounds_.y + (bounds_.height - side) / 2.0f;
    canvas.fill_rect({box_x, box_y, side, side}, theme::Field);
    if (checked_) {
        canvas.fill_rect({box_x + MarkInset, box_y + MarkInset, side - 2.0f * MarkInset,
                          side - 2.0f * MarkInset},
                         theme::Accent);
    }

    const Size text = canvas.measure_text(label_, style_.font_size);
    canvas.draw_text(label_, {box_x + side + LabelGap, bounds_.y + (bounds_.height - text.height) / 2.0f},
                     style_.font_size, style_.foreground);
}

void Checkbox::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    if (released && bounds_.contains(position)) {
        toggle();
    }
}

void Checkbox::click() {
    toggle();
    Widget::click();
}

void Checkbox::toggle() {
    checked_ = !checked_;
    if (on_change_) on_change_(checked_);
}

} // namespace ui
