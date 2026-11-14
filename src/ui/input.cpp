#include "ui/input.hpp"

namespace ui {

namespace {
constexpr float CaretWidth = 2.0f;
constexpr char FirstPrintable = 32;
constexpr char Delete = 127;
} // namespace

Size Input::preferred_size(const Canvas& canvas) const {
    const std::string& shown = value_.empty() ? placeholder_ : value_;
    return resolve_size(canvas.measure_text(shown, style_.font_size));
}

void Input::draw(Canvas& canvas) const {
    const Color border = focused_ ? style_.focus_border_color.value_or(style_.border_color)
                                  : style_.border_color;
    draw_box(canvas, style_.background, border);

    const bool showing_placeholder = value_.empty();
    const std::string& shown = showing_placeholder ? placeholder_ : value_;
    const Color text_color = showing_placeholder ? theme::Muted : style_.foreground;
    draw_text_in_box(canvas, shown, text_color);

    if (!focused_) {
        return;
    }
    const Size measured = canvas.measure_text(value_, style_.font_size);
    canvas.fill_rect({bounds_.x + style_.padding.left + measured.width,
                      bounds_.y + (bounds_.height - measured.height) / 2.0f, CaretWidth,
                      measured.height},
                     style_.foreground);
}

void Input::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    if (!released) {
        return;
    }
    focused_ = bounds_.contains(position);
}

void Input::type_char(char character) {
    if (!focused_ || value_.size() >= max_length_) {
        return;
    }
    if (character < FirstPrintable || character == Delete) {
        return;
    }
    value_.push_back(character);
    if (on_change_) on_change_(value_);
}

void Input::erase_last() {
    if (!focused_ || value_.empty()) {
        return;
    }
    value_.pop_back();
    if (on_change_) on_change_(value_);
}

} // namespace ui
