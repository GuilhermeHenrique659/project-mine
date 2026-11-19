#include "ui/tab_bar.hpp"

#include "ui/box.hpp"

namespace ui {

TabBar::TabBar(std::vector<Tab> tabs, Style style, Style active_style, Style disabled_style,
               std::size_t active)
    : Composite(style), tab_style_(std::move(style)), active_style_(std::move(active_style)),
      disabled_style_(std::move(disabled_style)), tabs_(std::move(tabs)), active_(active) {
    BoxBuilder row = ui::row();
    row.gap(tab_style_.gap).align(Align::Stretch);
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        auto button = std::make_unique<Button>(tabs_[i].title, look_of(i), tabs_[i].enabled);
        button->set_on_click([this, i] { activate(i); });
        buttons_.push_back(button.get());
        row.child(std::move(button));
    }
    set_tree(row.build());
}

Style TabBar::look_of(std::size_t index) const {
    if (index == active_) {
        // The active tab does not change on hover.
        return active_style_.with_hover_background(active_style_.background)
            .with_text_align(TextAlign::Center);
    }
    const Style& look = tabs_[index].enabled ? tab_style_ : disabled_style_;
    return look.with_text_align(TextAlign::Center);
}

// The tabs never change after construction; activate() restyles them.
void TabBar::sync(const Canvas&) const {}

void TabBar::activate(std::size_t index) {
    if (index == active_) {
        return;
    }
    const std::size_t previous = active_;
    active_ = index;
    buttons_[previous]->set_style(look_of(previous));
    buttons_[index]->set_style(look_of(index));
    if (on_change_) {
        on_change_(active_);
    }
}

std::vector<Rect> TabBar::tab_bounds() const {
    std::vector<Rect> bounds;
    for (const Button* button : buttons_) {
        bounds.push_back(button->bounds());
    }
    return bounds;
}

} // namespace ui
