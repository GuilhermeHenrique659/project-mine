#include "ui/button.hpp"

namespace ui {

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
