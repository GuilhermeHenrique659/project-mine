#include "ui/label.hpp"

namespace ui {

Size Label::preferred_size(const Canvas& canvas) const {
    return resolve_size(canvas.measure_text(text_, style_.font_size));
}

void Label::draw(Canvas& canvas) const {
    draw_box(canvas, style_.background, style_.border_color);
    draw_text_in_box(canvas, text_, style_.foreground);
}

} // namespace ui
