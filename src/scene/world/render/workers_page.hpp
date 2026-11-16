#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "core/registry.hpp"
#include "ui/button.hpp"
#include "ui/widget.hpp"

namespace scene::world {

// The Workers page of the player menu, read from ::world::worker_report() on every
// layout: "Idle workers: N", then one row per workplace, "<name> (x, y) - a/n workers"
// with a "-" and a "+" button that ask to release / assign one worker through
// `on_assign(building, count)` ("No building needs workers" when there is none). The
// registry must outlive it.
class WorkersPage : public ui::Widget {
public:
    using AssignHandler = std::function<void(core::EntityId building, int count)>;

    WorkersPage(const core::Registry& registry, AssignHandler on_assign);

    ui::Size preferred_size(const ui::Canvas& canvas) const override;
    void layout(ui::Rect bounds, const ui::Canvas& canvas) override;
    void draw(ui::Canvas& canvas) const override;
    void handle_pointer(ui::Point position, bool released) override;

    // The buttons of the row of the i-th workplace (valid after layout()).
    const ui::Button& add_button(std::size_t row) const { return *rows_[row].add; }
    const ui::Button& remove_button(std::size_t row) const { return *rows_[row].remove; }

private:
    struct Row {
        std::unique_ptr<ui::Button> remove;
        std::unique_ptr<ui::Button> add;
    };

    const core::Registry& registry_;
    AssignHandler on_assign_;
    std::vector<Row> rows_;
};

} // namespace scene::world
