#include "ui/tab_bar.hpp"

#include <algorithm>

namespace ui {

TabBar::TabBar(std::vector<Tab> tabs, Style style, Style active_style, Style disabled_style,
               std::size_t active)
    : Widget(std::move(style)), active_style_(std::move(active_style)),
      disabled_style_(std::move(disabled_style)), tabs_(std::move(tabs)), active_(active),
      hovered_tab_(tabs_.size()) {}

Size TabBar::tab_size(const Canvas& canvas, const Tab& tab) const {
    const Size text = canvas.measure_text(tab.title, style_.font_size);
    return {text.width + style_.padding.horizontal(), text.height + style_.padding.vertical()};
}

Size TabBar::preferred_size(const Canvas& canvas) const {
    Size size;
    for (const Tab& tab : tabs_) {
        const Size one = tab_size(canvas, tab);
        size.width += one.width;
        size.height = std::max(size.height, one.height);
    }
    if (!tabs_.empty()) {
        size.width += style_.gap * static_cast<float>(tabs_.size() - 1);
    }
    return size;
}

void TabBar::layout(Rect bounds, const Canvas& canvas) {
    const Size size = preferred_size(canvas);
    bounds_ = {bounds.x, bounds.y, size.width, size.height};
    tab_bounds_.clear();
    float x = bounds.x;
    for (const Tab& tab : tabs_) {
        const float width = tab_size(canvas, tab).width;
        tab_bounds_.push_back({x, bounds.y, width, size.height});
        x += width + style_.gap;
    }
}

void TabBar::draw(Canvas& canvas) const {
    for (std::size_t i = 0; i < tab_bounds_.size(); ++i) {
        const Tab& tab = tabs_[i];
        const Rect& area = tab_bounds_[i];
        const Style& look = i == active_ ? active_style_ : tab.enabled ? style_ : disabled_style_;
        const bool hovered = i == hovered_tab_ && i != active_;
        const Color face =
            hovered ? look.hover_background.value_or(look.background) : look.background;
        draw_raised(canvas, area, face, look);
        const Size text = canvas.measure_text(tab.title, style_.font_size);
        canvas.draw_text(tab.title,
                         {area.x + (area.width - text.width) / 2.0f,
                          area.y + (area.height - text.height) / 2.0f},
                         style_.font_size, look.foreground);
    }
}

void TabBar::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    hovered_tab_ = tabs_.size();
    for (std::size_t i = 0; i < tab_bounds_.size(); ++i) {
        if (tabs_[i].enabled && tab_bounds_[i].contains(position)) {
            hovered_tab_ = i;
        }
    }
    if (!released || hovered_tab_ == tabs_.size() || hovered_tab_ == active_) {
        return;
    }
    active_ = hovered_tab_;
    if (on_change_) {
        on_change_(active_);
    }
}

} // namespace ui
