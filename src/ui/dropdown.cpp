#include "ui/dropdown.hpp"

#include <algorithm>

namespace ui {

namespace {
constexpr const char* ClosedArrow = "v";
constexpr const char* OpenArrow = "^";
} // namespace

Dropdown::Dropdown(std::vector<std::string> options, std::size_t selected, Style style,
                   ChangeHandler on_change)
    : Widget(std::move(style)), options_(std::move(options)), selected_(selected),
      on_change_(std::move(on_change)) {
    // An option: the header's face without its bevel (the list gets one bevel around it).
    Style option = style_;
    option.bevel.reset();
    option.text_align = TextAlign::Left;
    BoxBuilder list = column();
    list.align(Align::Stretch);
    for (const std::string& text : options_) {
        auto button = std::make_unique<Button>(text, option, true);
        option_buttons_.push_back(button.get());
        list.child(std::move(button));
    }
    list_ = list.build();
}

void Dropdown::layout(Rect bounds, const Canvas& canvas) {
    Widget::layout(bounds, canvas);
    // Each option is as big as the header, stacked right below it.
    for (Button* button : option_buttons_) {
        button->set_style(button->style().with_height(bounds.height));
    }
    list_->layout({bounds.x, bounds.y + bounds.height, bounds.width, 0.0f}, canvas);
}

void Dropdown::highlight() const {
    // The option under the pointer wins; with no pointer over the list, show the selected one.
    const std::size_t highlighted = hovered_option_.value_or(selected_);
    for (std::size_t i = 0; i < option_buttons_.size(); ++i) {
        option_buttons_[i]->set_focused(i == highlighted);
    }
}

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
    const Color border = style_.border_color;

    draw_box(canvas, background, border);
    draw_bevel(canvas);
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
    if (!open_ || options_.empty()) {
        return;
    }
    highlight();
    list_->draw(canvas);
    draw_bevel(canvas, list_->bounds());
}

void Dropdown::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);

    if (!open_) {
        hovered_option_.reset();
        open_ = released && bounds_.contains(position);
        return;
    }

    // The option buttons track the hover; their own clicks are not used, since a release
    // anywhere closes the list.
    list_->handle_pointer(position, false);
    hovered_option_.reset();
    for (std::size_t i = 0; i < option_buttons_.size(); ++i) {
        if (option_buttons_[i]->hovered()) {
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
    return index < option_buttons_.size() ? option_buttons_[index]->bounds() : Rect{};
}

} // namespace ui
