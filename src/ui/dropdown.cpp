#include "ui/dropdown.hpp"

#include <algorithm>

namespace ui {

namespace {
constexpr const char* ClosedArrow = "v";
constexpr const char* OpenArrow = "^";
} // namespace

Size Dropdown::preferred_size(const Canvas& canvas) const {
    Size content = canvas.measure_text(ClosedArrow, style_.font_size);
    float widest = 0.0f;
    for (const std::string& option : options_) {
        const Size text = canvas.measure_text(option, style_.font_size);
        widest = std::max(widest, text.width);
        content.height = std::max(content.height, text.height);
    }
    // Room for the option text plus the arrow on its right.
    content.width = widest + content.width + static_cast<float>(style_.font_size);
    return resolve_size(content);
}

void Dropdown::draw(Canvas& canvas) const {
    const bool active = hovered() || focused_ || open_;
    const Color background =
        active ? style_.hover_background.value_or(style_.background) : style_.background;
    const Color foreground =
        active ? style_.hover_foreground.value_or(style_.foreground) : style_.foreground;
    const Color border = focused_ ? style_.focus_border_color.value_or(style_.border_color)
                                  : style_.border_color;

    draw_box(canvas, background, border);
    if (selected_ < options_.size()) {
        draw_text_in_box(canvas, options_[selected_], foreground);
    }

    const Size arrow = canvas.measure_text(ClosedArrow, style_.font_size);
    canvas.draw_text(open_ ? OpenArrow : ClosedArrow,
                     {bounds_.x + bounds_.width - style_.padding.right - arrow.width,
                      bounds_.y + (bounds_.height - arrow.height) / 2.0f},
                     style_.font_size, foreground);
}

void Dropdown::draw_overlay(Canvas& canvas) const {
    if (!open_) {
        return;
    }
    // The option under the pointer wins; with no pointer over the list, show the selected one.
    const std::size_t highlighted = hovered_option_.value_or(selected_);
    for (std::size_t i = 0; i < options_.size(); ++i) {
        const Rect rect = option_bounds(i);
        const bool is_highlighted = i == highlighted;
        const Color background =
            is_highlighted ? style_.hover_background.value_or(theme::Accent) : theme::Field;
        const Color foreground =
            is_highlighted ? style_.hover_foreground.value_or(theme::Dark) : style_.foreground;

        canvas.fill_rect(rect, background);
        canvas.stroke_rect(rect, style_.border_color, style_.border_width);
        const Size text = canvas.measure_text(options_[i], style_.font_size);
        canvas.draw_text(options_[i],
                         {rect.x + style_.padding.left, rect.y + (rect.height - text.height) / 2.0f},
                         style_.font_size, foreground);
    }
}

void Dropdown::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);

    if (!open_) {
        hovered_option_.reset();
        open_ = released && bounds_.contains(position);
        return;
    }

    hovered_option_.reset();
    for (std::size_t i = 0; i < options_.size(); ++i) {
        if (option_bounds(i).contains(position)) {
            hovered_option_ = i;
            break;
        }
    }
    if (!released) {
        return;
    }

    open_ = false;
    const std::optional<std::size_t> chosen = hovered_option_;
    hovered_option_.reset();
    if (chosen) {
        select(*chosen);
    }
}

void Dropdown::click() {
    if (!options_.empty()) {
        select((selected_ + 1) % options_.size());
    }
    Widget::click();
}

void Dropdown::select(std::size_t index) {
    if (index >= options_.size() || index == selected_) {
        return;
    }
    selected_ = index;
    if (on_change_) on_change_(selected_);
}

Rect Dropdown::option_bounds(std::size_t index) const {
    return {bounds_.x, bounds_.y + bounds_.height * static_cast<float>(index + 1), bounds_.width,
            bounds_.height};
}

} // namespace ui
