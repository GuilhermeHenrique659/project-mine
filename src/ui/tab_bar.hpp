#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "ui/button.hpp"
#include "ui/composite.hpp"

namespace ui {

struct Tab {
    std::string title;
    bool enabled = true;
};

// A row of tabs, left to right, one of them active: a ui::row() of Buttons. Clicking an
// enabled tab that is not active makes it active and calls on_change(index). `style` is
// an enabled tab (face, hover face, border, bevel, padding around the title, gap between
// tabs, font size); the active tab is drawn with `active_style` (no hover change) and
// disabled ones, which ignore the pointer, with `disabled_style`. Titles are centred; all
// tabs are as high as the tallest. Hit-testing uses the last layout().
class TabBar : public Composite {
public:
    using ChangeHandler = std::function<void(std::size_t)>;

    TabBar(std::vector<Tab> tabs, Style style, Style active_style, Style disabled_style,
           std::size_t active = 0);

    void set_on_change(ChangeHandler handler) { on_change_ = std::move(handler); }

    std::size_t active() const { return active_; }
    const std::vector<Tab>& tabs() const { return tabs_; }
    // Screen bounds of each tab, from the last layout().
    std::vector<Rect> tab_bounds() const;

private:
    void sync(const Canvas& canvas) const override;
    void activate(std::size_t index);
    // The look of tab `index` now (active, enabled or disabled).
    Style look_of(std::size_t index) const;

    Style tab_style_;
    Style active_style_;
    Style disabled_style_;
    std::vector<Tab> tabs_;
    std::size_t active_;
    ChangeHandler on_change_;
    std::vector<Button*> buttons_; // owned by the tree
};

} // namespace ui
