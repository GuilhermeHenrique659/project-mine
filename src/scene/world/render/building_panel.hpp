#pragma once

#include <memory>
#include <string>
#include <vector>

#include "input/pointer.hpp"
#include "scene/world/render/inventory_grid.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"
#include "ui/button.hpp"

namespace scene::world {

// The item transfer with a building (WorldModel::transfer_target()): a panel in the
// middle of the screen with the player's inventory on top and, below it, the building's
// name, its details (state, power; see building_details()) and its inventory, at most
// four rows at once: the mouse wheel over it scrolls a bigger one (a storage).
// Clicking a filled slot moves that whole stack to the other inventory (as much as fits,
// through WorldModel::request_transfer()); the red X closes it. Visible while a transfer
// is open and the game is not paused. It reads the mouse itself in update(), using the
// bounds of the last layout. The model must outlive it.
class BuildingPanel : public Renderer {
public:
    BuildingPanel(WorldModel& model, std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    void update() override;
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

    // Places the panel, both grids and the close button (draw() does it before painting).
    void layout(const ui::Canvas& canvas, ui::Size screen) const;

    ui::Rect player_slot_bounds(std::size_t index) const;
    ui::Rect building_slot_bounds(std::size_t index) const;
    ui::Rect close_bounds() const { return close_button_->bounds(); }

private:
    // Moves the stack in `slot` of `from` to `to`; nothing when the slot is empty.
    void move_stack(core::EntityId from, core::EntityId to, std::size_t slot) const;
    // The grid of the current target, recreated when the target changes.
    InventoryGrid& building_grid() const;

    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    std::unique_ptr<InventoryGrid> player_grid_;
    std::unique_ptr<ui::Button> close_button_;
    // Layout state, recomputed by every layout(); mutable because draw() is const.
    mutable std::unique_ptr<InventoryGrid> building_grid_;
    mutable core::EntityId building_ = 0;
    mutable ui::Rect panel_;
    mutable ui::Point player_title_;
    mutable ui::Point building_title_;
    // The building's detail lines (see building_details()) and where the first one goes.
    mutable std::vector<std::string> details_;
    mutable float details_top_ = 0.0f;
};

} // namespace scene::world
