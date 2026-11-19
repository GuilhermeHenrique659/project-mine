#pragma once

#include <memory>
#include <vector>

#include "input/pointer.hpp"
#include "scene/world/render/inventory_grid.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"
#include "ui/box.hpp"
#include "ui/button.hpp"
#include "ui/label.hpp"
#include "ui/scroll_box.hpp"

namespace scene::world {

// The item transfer with a building (WorldModel::transfer_target()): a panel in the
// middle of the screen with the player's inventory on top and, below it, the building's
// name, its details (state, power; see building_details()) and its inventory, at most
// four rows at once: the mouse wheel over it scrolls a bigger one (a storage).
// Clicking a filled slot moves that whole stack to the other inventory (as much as fits,
// through WorldModel::request_transfer()); the red X closes it. A building that recruits
// (a barracks) shows a "Recruit" button under its inventory: one soldier per click
// (WorldModel::request_recruit()). Visible while a transfer
// is open and the game is not paused. It reads the mouse itself in update(), using the
// bounds of the last layout. The model must outlive it.
class BuildingPanel : public Renderer {
public:
    BuildingPanel(WorldModel& model, std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    void update() override;
    // Modal: while visible it takes the pointer everywhere.
    bool captures_pointer(ui::Point) const override { return true; }
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

    // Places the panel, both grids and the close button (draw() does it before painting).
    void layout(const ui::Canvas& canvas, ui::Size screen) const;

    ui::Rect player_slot_bounds(std::size_t index) const;
    ui::Rect building_slot_bounds(std::size_t index) const;
    ui::Rect close_bounds() const { return close_button_->bounds(); }
    // Empty while the button is hidden (the target does not recruit).
    ui::Rect recruit_bounds() const;
    // Whether the current building recruits soldiers (the Recruit button is shown).
    bool recruit_visible() const;

private:
    // Moves the stack in `slot` of `from` to `to`; nothing when the slot is empty.
    void move_stack(core::EntityId from, core::EntityId to, std::size_t slot) const;
    // The grid of the current target, recreated when the target changes.
    InventoryGrid& building_grid() const;
    // Brings the widget tree up to date with the current target: it is built once per
    // building (and again only when its number of detail lines changes); every other frame
    // just updates the texts and shows or hides the grid, details and Recruit button.
    void sync_tree() const;
    // A tree with `detail_lines` empty detail labels. It points at building_grid_, so
    // building_grid() drops it whenever it replaces that grid.
    std::unique_ptr<ui::Box> build_tree(std::size_t detail_lines) const;

    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    std::unique_ptr<InventoryGrid> player_grid_;
    std::unique_ptr<ui::Button> close_button_;
    std::unique_ptr<ui::Button> recruit_button_;
    // Layout state, recomputed by every layout(); mutable because draw() is const.
    mutable std::unique_ptr<InventoryGrid> building_grid_;
    mutable core::EntityId building_ = 0;
    mutable std::unique_ptr<ui::Box> root_;
    // Parts of root_ updated in place (owned by it).
    mutable ui::Label* building_title_ = nullptr;
    mutable std::vector<ui::Label*> detail_labels_;
    mutable ui::Widget* details_ = nullptr;
    mutable ui::Widget* recruit_row_ = nullptr;
    // Shows four rows of the building's grid at most (a storage scrolls).
    mutable ui::ScrollBox* building_view_ = nullptr;
};

} // namespace scene::world
