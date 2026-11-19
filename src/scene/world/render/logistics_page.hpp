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
#include "ui/composite.hpp"
#include "ui/label.hpp"

namespace scene::world {

// The Logistics page of the player menu, read from ::world::logistics_report() on every
// layout: "Logistic routes: N", then the routes grouped by destination building type,
// each group under a heading button "[-] <type name> (count)" that hides its routes
// (then "[+] ...") and shows them again on the next click; one row per shown route.
// Every line is laid out (the player menu scrolls the page). Each row reads
// "<from> (x, y) -> <to> (x, y) - N tiles - a workers" ("unreachable" without a path) with
// two checkbox columns "Take" and "Put" (each click asks for the mode with that direction
// switched through `on_mode`: both checked = Both; the last checked box cannot be
// unchecked), then "-" and "+" (ask to release / assign one hauler in that mode through
// `on_assign`). A route with haulers shows their mode; one without shows the mode last
// chosen here (Take at first). A hint to build a storage when there is no route. A widget
// tree kept between frames: rebuilt only when the lines change (a route added or removed, a
// group hidden or shown), otherwise only texts and checkboxes are updated. The registry and
// the network must outlive it.
class LogisticsPage : public ui::Composite {
public:
    using AssignHandler = std::function<void(::world::RouteKey, ::world::HaulMode, int count)>;
    using ModeHandler = std::function<void(::world::RouteKey, ::world::HaulMode)>;

    LogisticsPage(const core::Registry& registry, const ::world::RouteNetwork& network,
                  AssignHandler on_assign, ModeHandler on_mode);

    // The i-th shown group's heading button (valid after layout()).
    const ui::Button& heading_button(std::size_t group) const { return *headings_[group]; }

    // The buttons of the i-th shown route's row (valid after layout()).
    const ui::Checkbox& take_checkbox(std::size_t row) const { return *rows_[row].take; }
    const ui::Checkbox& put_checkbox(std::size_t row) const { return *rows_[row].put; }
    const ui::Button& remove_button(std::size_t row) const { return *rows_[row].remove; }
    const ui::Button& add_button(std::size_t row) const { return *rows_[row].add; }

private:
    // One line of the list: a group's heading (by destination type) or a route's row.
    struct Line {
        bool heading = false;
        ::world::BuildingType type{};
        ::world::RouteKey route{};
        bool operator==(const Line&) const = default;
    };
    struct Row {
        ui::Label* text = nullptr;
        ui::Checkbox* take = nullptr;
        ui::Checkbox* put = nullptr;
        ui::Button* remove = nullptr;
        ui::Button* add = nullptr;
    };

    void sync(const ui::Canvas& canvas) const override;
    void build_tree(const std::vector<Line>& lines) const;
    ::world::HaulMode chosen_mode(::world::RouteKey route) const;
    // Asks for `route`'s mode with its directions set to take / put (refused when neither).
    void change_mode(::world::RouteKey route, bool take, bool put) const;

    const core::Registry& registry_;
    const ::world::RouteNetwork& network_;
    AssignHandler on_assign_;
    ModeHandler on_mode_;
    // Page-local UI state, changed by the controls' handlers.
    mutable std::set<::world::BuildingType> hidden_;
    // The mode picked per route (from, to), for routes without haulers yet.
    mutable std::map<std::pair<core::EntityId, core::EntityId>, ::world::HaulMode> modes_;
    // The tree's parts (owned by it), from the last build.
    mutable std::vector<Line> lines_;
    mutable bool built_ = false;
    mutable ui::Label* count_ = nullptr;
    mutable std::vector<ui::Button*> headings_;
    mutable std::vector<Row> rows_; // only the routes of shown groups
};

} // namespace scene::world
