#include "scene/world/render/site_work.hpp"

namespace scene::world {

SiteWork::SiteWork(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model), pointer_(std::move(pointer)) {}

bool SiteWork::visible() const { return !model_.is_paused() && !model_.is_player_menu_open(); }

void SiteWork::update() {
    if (!pointer_->down()) {
        model_.work_on(std::nullopt);
        return;
    }
    const input::PointerPosition at = pointer_->position();
    model_.work_on(
        model_.camera().projection(screen_.width, screen_.height).tile_at({at.x, at.y}));
}

void SiteWork::draw(ui::Canvas&, ui::Size screen) const { screen_ = screen; }

} // namespace scene::world
