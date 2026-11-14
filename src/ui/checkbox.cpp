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
    const bool active = focused_ || hovered();
    const Color background =
        active ? style_.hover_background.value_or(style_.background) : style_.background;
    draw_box(canvas, background, style_.border_color);
    draw_bevel(canvas);

    const float side = static_cast<float>(style_.font_size);
    const float box_x = bounds_.x + style_.padding.left;
    const float box_y = bounds_.y + (bounds_.height - side) / 2.0f;
    canvas.fill_rect({box_x, box_y, side, side}, theme::Field);
    canvas.stroke_rect({box_x, box_y, side, side}, theme::Outline, 2.0f);
    if (checked_) {
        canvas.fill_rect({box_x + MarkInset, box_y + MarkInset, side - 2.0f * MarkInset,
                          side - 2.0f * MarkInset},
                         theme::GreenFace);
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
