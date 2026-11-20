#pragma once

#include <memory>
#include <vector>

#include "input/pointer.hpp"
#include "scene/world/render/inventory_grid.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"
#include "ui/box.hpp"
#include "ui/button.hpp"
#include "ui/scroll_box.hpp"
#include "ui/tab_bar.hpp"

namespace scene::world {

// The player menu (Tab): a panel covering almost the whole screen with a tab bar on top
// (SolidInventory, active; Energy; Workers; Statistics, disabled until it exists; Logistics) and
// the active tab's page below it (SolidInventory: the player's inventory grid; Energy: the
// EnergyPage; Workers: the WorkersPage; Logistics: the LogisticsPage), and an X button in the
// top-right corner that closes it. Visible while the model's player menu is open and the
// game is not paused; the world keeps running behind it. It reads the mouse itself in
// update() through the injected pointer, using the bounds of the last draw(). The model
// must outlive the menu.
class PlayerMenu : public Renderer {
public:
    // Part of the screen the panel covers, in each direction.
    static constexpr float ScreenCoverage = 0.9f;

    PlayerMenu(WorldModel& model, std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    void update() override;
    // Modal: while visible it takes the pointer everywhere.
    bool captures_pointer(ui::Point) const override { return true; }
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

    // Places the panel, the tabs and the page on a screen of the given size (draw() does
    // it before painting).
    void layout(const ui::Canvas& canvas, ui::Size screen) const;

    const ui::TabBar& tab_bar() const { return *tab_bar_; }
    const ui::Button& close_button() const { return *close_button_; }
    const ui::Rect& panel_bounds() const { return panel_; }
    // The scroll view of the active page, null for a tab without one.
    const ui::ScrollBox* page_view() const;

private:
    PlayerMenu(const PlayerMenu&) = delete;
    PlayerMenu& operator=(const PlayerMenu&) = delete;

    std::unique_ptr<ui::Box> build_tree(ui::Size panel) const;

    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    std::unique_ptr<ui::TabBar> tab_bar_;
    std::unique_ptr<ui::Button> close_button_;
    // One page per tab, null for the tabs without content yet.
    std::vector<std::unique_ptr<ui::Widget>> pages_;
    // One scroll view per page (null for a page not available yet), in tab order.
    std::vector<std::unique_ptr<ui::ScrollBox>> page_views_;
    // Layout state; mutable because draw() is const. The tree is rebuilt only when the
    // screen size changes.
    mutable ui::Rect panel_;
    mutable ui::Size screen_;
    mutable std::unique_ptr<ui::Box> root_;
};

} // namespace scene::world
