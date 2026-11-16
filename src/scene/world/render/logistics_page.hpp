#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/widget.hpp"

namespace scene::world {

// The Logistics page of the player menu, read from ::world::logistics_report() on every
// layout: "Logistic routes: N", then the routes grouped by destination building type,
// each group under a heading button "[-] <type name> (count)" that hides its routes
// (then "[+] ...") and shows them again on the next click; one row per shown route.
// Lines that do not fit the page's height are scrolled to with scroll(), with a
// scrollbar on the right while some are hidden (like InventoryGrid). Each row reads
// "<from> (x, y) -> <to> (x, y) - N tiles - a workers" ("unreachable" without a path) with
// two checkbox columns "Take" and "Put" (each click asks for the mode with that direction
// switched through `on_mode`: both checked = Both; the last checked box cannot be
// unchecked), then "-" and "+" (ask to release / assign one hauler in that mode through
// `on_assign`). A route with haulers shows their mode; one without shows the mode last
// chosen here (Take at first). A hint to build a storage when there is no route. The
// registry and the network must outlive it.
class LogisticsPage : public ui::Widget {
public:
    using AssignHandler = std::function<void(::world::RouteKey, ::world::HaulMode, int count)>;
    using ModeHandler = std::function<void(::world::RouteKey, ::world::HaulMode)>;

    LogisticsPage(const core::Registry& registry, const ::world::RouteNetwork& network,
                  AssignHandler on_assign, ModeHandler on_mode);

    ui::Size preferred_size(const ui::Canvas& canvas) const override;
    void layout(ui::Rect bounds, const ui::Canvas& canvas) override;
    void draw(ui::Canvas& canvas) const override;
    void handle_pointer(ui::Point position, bool released) override;

    // Moves the view by mouse wheel notches (positive = up), a line per notch, never past
    // the first or the last lines.
    void scroll(float notches);
    // The first shown line (headings and rows count as one line each).
    std::size_t first_line() const { return first_line_; }

    // The i-th shown group's heading button (valid after layout()).
    const ui::Button& heading_button(std::size_t group) const { return *headings_[group]; }

    // The buttons of the i-th shown route's row (valid after layout()).
    const ui::Checkbox& take_checkbox(std::size_t row) const { return *rows_[row].take; }
    const ui::Checkbox& put_checkbox(std::size_t row) const { return *rows_[row].put; }
    const ui::Button& remove_button(std::size_t row) const { return *rows_[row].remove; }
    const ui::Button& add_button(std::size_t row) const { return *rows_[row].add; }

private:
    struct Row {
        std::size_t entry = 0; // index in the report
        std::unique_ptr<ui::Checkbox> take;
        std::unique_ptr<ui::Checkbox> put;
        std::unique_ptr<ui::Button> remove;
        std::unique_ptr<ui::Button> add;
    };

    ::world::HaulMode chosen_mode(::world::RouteKey route) const;
    void draw_scrollbar(ui::Canvas& canvas) const;

    const core::Registry& registry_;
    const ::world::RouteNetwork& network_;
    AssignHandler on_assign_;
    ModeHandler on_mode_;
    std::vector<Row> rows_; // only the routes of shown groups
    std::vector<std::unique_ptr<ui::Button>> headings_;
    std::set<::world::BuildingType> hidden_;
    // Scrolling, from the last layout: lines that fit, the largest first line, line height
    // and where the first line starts.
    std::size_t first_line_ = 0;
    std::size_t shown_lines_ = 0;
    std::size_t last_first_line_ = 0;
    float line_height_ = 0.0f;
    float list_top_ = 0.0f;
    // The mode picked per route (from, to), for routes without haulers yet.
    std::map<std::pair<core::EntityId, core::EntityId>, ::world::HaulMode> modes_;
};

} // namespace scene::world
