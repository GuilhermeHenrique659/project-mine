#pragma once

#include <memory>

#include "input/pointer.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"
#include "ui/button.hpp"

namespace scene::world {

// The vehicle controls of the HUD: an "Unload" button in the bottom-left corner of the
// screen, shown while the game is not paused and some vehicle the player commands carries
// units (WorldModel::unloadable_vehicles()). Clicking it asks the model to let them all out
// (WorldModel::unload_vehicles()). It reads the mouse itself in update() through the
// injected pointer, using the bounds of the last layout. The model must outlive it.
class VehicleHud : public Renderer {
public:
    VehicleHud(WorldModel& model, std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    void update() override;
    bool captures_pointer(ui::Point position) const override;
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

    // Places the button on a screen of the given size (draw() does it before painting).
    void layout(const ui::Canvas& canvas, ui::Size screen) const;
    const ui::Rect& unload_bounds() const { return button_->bounds(); }

private:
    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    std::unique_ptr<ui::Button> button_;
};

} // namespace scene::world
