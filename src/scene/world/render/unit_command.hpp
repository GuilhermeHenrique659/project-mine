#pragma once

#include <memory>

#include "input/pointer.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"

namespace scene::world {

// How far (in pixels) the pointer must move with the left button held for it to be a box
// selection rather than a click.
inline constexpr float SelectionDragThreshold = 6.0f;

// Commands units with the mouse: holding the left button and dragging draws a box, and
// releasing selects the soldiers inside it (WorldModel::select_units()); a plain left click
// clears the selection (and SiteWork still builds or opens a transfer with it). A right click orders
// the selection to walk to the tile under the pointer (WorldModel::order_move()). Hidden
// while paused, while the player menu is open or while a transfer is open. The model must
// outlive it.
class UnitCommand : public Renderer {
public:
    UnitCommand(WorldModel& model, std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    // While a box is being dragged, so SiteWork below neither works nor opens a transfer.
    bool captures_pointer(ui::Point) const override;
    void update() override;
    // Draws the selection box while dragging and records the screen size used to project.
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    mutable ui::Size screen_;
    bool pressed_ = false;
    bool dragged_ = false;
    input::PointerPosition anchor_;
    input::PointerPosition current_;
};

} // namespace scene::world
