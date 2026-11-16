#pragma once

#include <memory>
#include <vector>

#include "input/pointer.hpp"
#include "scene/world/render/inventory_grid.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"
#include "ui/button.hpp"
#include "ui/tab_bar.hpp"

namespace scene::world {

class LogisticsPage;

// The player menu (Tab): a panel covering almost the whole screen with a tab bar on top
// (Inventory, active; Energy; Workers; Statistics, disabled until it exists; Logistics) and
// the active tab's page below it (Inventory: the player's inventory grid; Energy: the
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
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

    // Places the panel, the tabs and the page on a screen of the given size (draw() does
    // it before painting).
    void layout(const ui::Canvas& canvas, ui::Size screen) const;

    const ui::TabBar& tab_bar() const { return *tab_bar_; }
    const ui::Button& close_button() const { return *close_button_; }
    const ui::Rect& panel_bounds() const { return panel_; }

private:
    PlayerMenu(const PlayerMenu&) = delete;
    PlayerMenu& operator=(const PlayerMenu&) = delete;

    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    // Layout state, recomputed by every layout(); mutable because draw() is const.
    std::unique_ptr<ui::TabBar> tab_bar_;
    std::unique_ptr<ui::Button> close_button_;
    // One page per tab, null for the tabs without content yet.
    std::vector<std::unique_ptr<ui::Widget>> pages_;
    // The Logistics page in pages_ (not owning), for the mouse wheel.
    LogisticsPage* logistics_ = nullptr;
    mutable ui::Rect panel_;
};

} // namespace scene::world
