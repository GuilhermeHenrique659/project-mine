#pragma once

#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "ui/button.hpp"
#include "ui/widget.hpp"

namespace scene::world {

// The Logistics page of the player menu, read from ::world::logistics_report() on every
// layout: "Logistic routes: N", then one row per logistic route,
// "<from> (x, y) -> <to> (x, y) - N tiles - a workers" ("unreachable" without a path) with
// three buttons: the route's mode ("Take" / "Put"; a click asks to switch it through
// `on_mode`), "-" and "+" (ask to release / assign one hauler in that mode through
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

    // The buttons of the i-th route's row (valid after layout()).
    const ui::Button& mode_button(std::size_t row) const { return *rows_[row].mode; }
    const ui::Button& remove_button(std::size_t row) const { return *rows_[row].remove; }
    const ui::Button& add_button(std::size_t row) const { return *rows_[row].add; }

private:
    struct Row {
        std::unique_ptr<ui::Button> mode;
        std::unique_ptr<ui::Button> remove;
        std::unique_ptr<ui::Button> add;
    };

    ::world::HaulMode chosen_mode(::world::RouteKey route) const;

    const core::Registry& registry_;
    const ::world::RouteNetwork& network_;
    AssignHandler on_assign_;
    ModeHandler on_mode_;
    std::vector<Row> rows_;
    // The mode picked per route (from, to), for routes without haulers yet.
    std::map<std::pair<core::EntityId, core::EntityId>, ::world::HaulMode> modes_;
};

} // namespace scene::world
