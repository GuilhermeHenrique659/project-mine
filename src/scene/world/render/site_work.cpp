#include "scene/world/render/site_work.hpp"

namespace scene::world {

SiteWork::SiteWork(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model), pointer_(std::move(pointer)) {}

bool SiteWork::visible() const {
    return !model_.is_paused() && !model_.is_player_menu_open() && model_.transfer_target() == 0;
}

void SiteWork::update() {
    const input::PointerPosition at = pointer_->position();
    const ::world::TileCoord tile =
        model_.camera().projection(screen_.width, screen_.height).tile_at({at.x, at.y});
    // A click on a finished building with an inventory opens the transfer with it.
    if (pointer_->released()) {
        model_.open_transfer(tile);
    }
    if (!pointer_->down()) {
        model_.work_on(std::nullopt);
        return;
    }
    model_.work_on(tile);
}

void SiteWork::draw(ui::Canvas&, ui::Size screen) const { screen_ = screen; }

} // namespace scene::world
