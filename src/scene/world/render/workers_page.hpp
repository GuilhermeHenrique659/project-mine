#pragma once

#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/worker_report.hpp"
#include "ui/button.hpp"
#include "ui/widget.hpp"

namespace scene::world {

// The Workers page of the player menu, read from ::world::worker_report() on every
// layout: "Idle workers: N", then the workplaces grouped by building type, each group
// under a heading button "[-] <type name> (count)" that hides its workplaces (then
// "[+] ...") and shows them again on the next click; one row per shown workplace,
// "<name> (x, y) - a/n workers" with a "-" and a "+" button that ask to release / assign
// one worker through `on_assign(building, count)` ("No building needs workers" when there
// is none). Lines that do not fit the page's height are scrolled to with scroll(), with a
// scrollbar on the right while some are hidden (like LogisticsPage). The registry must
// outlive it.
class WorkersPage : public ui::Widget {
public:
    using AssignHandler = std::function<void(core::EntityId building, int count)>;

    WorkersPage(const core::Registry& registry, AssignHandler on_assign);

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

    // The buttons of the i-th shown workplace's row (valid after layout()).
    const ui::Button& add_button(std::size_t row) const { return *rows_[row].add; }
    const ui::Button& remove_button(std::size_t row) const { return *rows_[row].remove; }

private:
    struct Row {
        std::size_t entry = 0; // index in the grouped workplaces
        std::unique_ptr<ui::Button> remove;
        std::unique_ptr<ui::Button> add;
    };

    // The report with its workplaces grouped by building type (registry order within one).
    ::world::WorkerReport grouped_report() const;
    void draw_scrollbar(ui::Canvas& canvas) const;

    const core::Registry& registry_;
    AssignHandler on_assign_;
    std::vector<Row> rows_; // only the workplaces of shown groups
    std::vector<std::unique_ptr<ui::Button>> headings_;
    std::set<::world::BuildingType> hidden_;
    // Scrolling, from the last layout: lines that fit, the largest first line, line height
    // and where the first line starts.
    std::size_t first_line_ = 0;
    std::size_t shown_lines_ = 0;
    std::size_t last_first_line_ = 0;
    float line_height_ = 0.0f;
    float list_top_ = 0.0f;
};

} // namespace scene::world
