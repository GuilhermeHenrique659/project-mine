#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "ui/widget.hpp"

namespace ui {

struct Tab {
    std::string title;
    bool enabled = true;
};

// A row of tabs, left to right, one of them active, each drawn like a button. Clicking an
// enabled tab that is not active makes it active and calls on_change(index). `style` is
// an enabled tab (face, hover face, border, bevel, padding around the title, gap between
// tabs, font size); the active tab is drawn with `active_style` and disabled ones, which
// ignore the pointer, with `disabled_style`. Hit-testing uses the last layout().
class TabBar : public Widget {
public:
    using ChangeHandler = std::function<void(std::size_t)>;

    TabBar(std::vector<Tab> tabs, Style style, Style active_style, Style disabled_style,
           std::size_t active = 0);

    Size preferred_size(const Canvas& canvas) const override;
    void layout(Rect bounds, const Canvas& canvas) override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;

    void set_on_change(ChangeHandler handler) { on_change_ = std::move(handler); }

    std::size_t active() const { return active_; }
    const std::vector<Tab>& tabs() const { return tabs_; }
    // Screen bounds of each tab, from the last layout().
    const std::vector<Rect>& tab_bounds() const { return tab_bounds_; }

private:
    Size tab_size(const Canvas& canvas, const Tab& tab) const;

    Style active_style_;
    Style disabled_style_;
    std::vector<Tab> tabs_;
    std::vector<Rect> tab_bounds_;
    std::size_t active_;
    // The tab under the pointer, or tabs_.size() for none.
    std::size_t hovered_tab_;
    ChangeHandler on_change_;
};

} // namespace ui
