#pragma once

#include <functional>
#include <set>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/worker_report.hpp"
#include "ui/button.hpp"
#include "ui/composite.hpp"
#include "ui/label.hpp"

namespace scene::world {

// The Workers page of the player menu, read from ::world::worker_report() on every
// layout: "Idle workers: N", then the workplaces grouped by building type, each group
// under a heading button "[-] <type name> (count)" that hides its workplaces (then
// "[+] ...") and shows them again on the next click; one row per shown workplace,
// "<name> (x, y) - a/n workers" with a "-" and a "+" button that ask to release / assign
// one worker through `on_assign(building, count)` ("No building needs workers" when there
// is none). Every line is laid out (the player menu scrolls the page). A widget tree kept
// between frames: rebuilt only when the lines change (a workplace added or removed, a group
// hidden or shown), otherwise only its texts are updated. The registry must outlive it.
class WorkersPage : public ui::Composite {
public:
    using AssignHandler = std::function<void(core::EntityId building, int count)>;

    WorkersPage(const core::Registry& registry, AssignHandler on_assign);

    // The i-th shown group's heading button (valid after layout()).
    const ui::Button& heading_button(std::size_t group) const { return *headings_[group]; }

    // The buttons of the i-th shown workplace's row (valid after layout()).
    const ui::Button& add_button(std::size_t row) const { return *rows_[row].add; }
    const ui::Button& remove_button(std::size_t row) const { return *rows_[row].remove; }

private:
    // One line of the list: a group's heading (by building type) or a workplace's row.
    struct Line {
        bool heading = false;
        ::world::BuildingType type{};
        core::EntityId id = 0;
        bool operator==(const Line&) const = default;
    };
    struct Row {
        ui::Label* text = nullptr;
        ui::Button* remove = nullptr;
        ui::Button* add = nullptr;
    };

    void sync(const ui::Canvas& canvas) const override;
    void build_tree(const std::vector<Line>& lines) const;
    // The report with its workplaces grouped by building type (registry order within one).
    ::world::WorkerReport grouped_report() const;

    const core::Registry& registry_;
    AssignHandler on_assign_;
    // Groups folded by their heading; page-local UI state, changed by the headings.
    mutable std::set<::world::BuildingType> hidden_;
    // The tree's parts (owned by it), from the last build.
    mutable std::vector<Line> lines_;
    mutable bool built_ = false;
    mutable ui::Label* idle_ = nullptr;
    mutable std::vector<ui::Button*> headings_;
    mutable std::vector<Row> rows_; // only the workplaces of shown groups
};

} // namespace scene::world
