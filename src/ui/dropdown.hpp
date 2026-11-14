#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ui/focus_group.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"

namespace ui {

// Single-choice list. Closed, it shows the selected option; clicking it opens the list of
// options right below. Clicking an option selects it (notifying on_change with its index)
// and closes the list; clicking anywhere else just closes it. From the keyboard,
// activating it cycles to the next option, wrapping around.
// The list is a popup: it is drawn through draw_overlay() and, while open, blocks_pointer().
class Dropdown : public Widget, public Focusable {
public:
    using ChangeHandler = std::function<void(std::size_t)>;

    Dropdown(std::vector<std::string> options, std::size_t selected, Style style,
             ChangeHandler on_change)
        : Widget(std::move(style)),
          options_(std::move(options)),
          selected_(selected),
          on_change_(std::move(on_change)) {}

    Size preferred_size(const Canvas& canvas) const override;
    void draw(Canvas& canvas) const override;
    void draw_overlay(Canvas& canvas) const override;
    bool blocks_pointer() const override { return open_; }
    void handle_pointer(Point position, bool released) override;
    void click() override;

    void set_focused(bool focused) override { focused_ = focused; }
    void activate() override { click(); }

    // Selects an option; notifies only when it differs from the current one.
    void select(std::size_t index);
    // Bounds of an option in the popup, stacked below the header; from the last layout.
    Rect option_bounds(std::size_t index) const;

    // Option under the pointer while the list is open.
    std::optional<std::size_t> hovered_option() const { return hovered_option_; }
    bool is_open() const { return open_; }
    std::size_t selected() const { return selected_; }
    std::size_t option_count() const { return options_.size(); }

private:
    std::vector<std::string> options_;
    std::size_t selected_;
    ChangeHandler on_change_;
    std::optional<std::size_t> hovered_option_;
    bool open_ = false;
    bool focused_ = false;
};

class DropdownBuilder : public HandlerBuilder<DropdownBuilder> {
public:
    DropdownBuilder() : style_(theme::dropdown_style()) {}

    DropdownBuilder& style(Style style) { style_ = std::move(style); return *this; }
    DropdownBuilder& options(std::vector<std::string> options) { options_ = std::move(options); return *this; }
    DropdownBuilder& selected(std::size_t index) { selected_ = index; return *this; }
    DropdownBuilder& on_change(Dropdown::ChangeHandler handler) { on_change_ = std::move(handler); return *this; }
    std::unique_ptr<Dropdown> build() {
        auto dropdown = std::make_unique<Dropdown>(std::move(options_), selected_, style_,
                                                   std::move(on_change_));
        apply_handlers(*dropdown);
        return dropdown;
    }

private:
    Style style_;
    std::vector<std::string> options_;
    std::size_t selected_ = 0;
    Dropdown::ChangeHandler on_change_;
};

inline DropdownBuilder dropdown() { return DropdownBuilder(); }

} // namespace ui
