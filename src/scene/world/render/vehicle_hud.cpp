#include "scene/world/render/vehicle_hud.hpp"

namespace scene::world {

namespace {
// Gap between the button and the screen edges.
constexpr float Margin = 16.0f;
} // namespace

VehicleHud::VehicleHud(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model),
      pointer_(std::move(pointer)),
      button_(std::make_unique<ui::Button>("Unload", ui::theme::button_style(), true)) {
    button_->set_on_click([this] { model_.unload_vehicles(); });
}

bool VehicleHud::visible() const {
    return !model_.is_paused() && !model_.unloadable_vehicles().empty();
}

void VehicleHud::update() {
    const input::PointerPosition at = pointer_->position();
    button_->handle_pointer({at.x, at.y}, pointer_->released());
}

bool VehicleHud::captures_pointer(ui::Point position) const {
    return button_->bounds().contains(position);
}

void VehicleHud::layout(const ui::Canvas& canvas, ui::Size screen) const {
    const ui::Size size = button_->preferred_size(canvas);
    button_->layout({Margin, screen.height - size.height - Margin, size.width, size.height}, canvas);
}

void VehicleHud::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    button_->draw(canvas);
}

} // namespace scene::world
