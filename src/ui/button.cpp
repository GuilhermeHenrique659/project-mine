#include "ui/button.hpp"

namespace ui {

namespace {
// Gap around an icon, as a share of the button's height.
constexpr float IconInsetRatio = 0.15f;
} // namespace

Size Button::preferred_size(const Canvas& canvas) const {
    return resolve_size(canvas.measure_text(text_, style_.font_size));
}

void Button::draw(Canvas& canvas) const {
    const bool active = enabled_ && (hovered() || focused_);
    const Color background =
        active ? style_.hover_background.value_or(style_.background) : style_.background;
    const Color foreground =
        active ? style_.hover_foreground.value_or(style_.foreground) : style_.foreground;

    draw_box(canvas, background, style_.border_color);
    draw_bevel(canvas);
    draw_text_in_box(canvas, text_, foreground);
    if (!icon_) {
        return;
    }
    const float inset = bounds_.height * IconInsetRatio;
    const float side = bounds_.height - 2.0f * inset;
    icon_(canvas, {bounds_.x + inset * 2.0f, bounds_.y + inset, side, side});
}

void Button::click() {
    if (!enabled_) {
        return;
    }
    Widget::click();
}

void Button::handle_pointer(Point position, bool released) {
    if (!enabled_) {
        return;
    }
    Widget::handle_pointer(position, released);
}

} // namespace ui
