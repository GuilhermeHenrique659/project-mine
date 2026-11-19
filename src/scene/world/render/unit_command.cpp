#include "scene/world/render/unit_command.hpp"

#include <algorithm>
#include <cmath>

namespace scene::world {

namespace {
constexpr ui::Color BoxFill{230, 160, 40, 50};
constexpr ui::Color BoxEdge{230, 160, 40, 220};
constexpr float BoxEdgeWidth = 1.0f;
} // namespace

UnitCommand::UnitCommand(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model), pointer_(std::move(pointer)) {}

bool UnitCommand::visible() const {
    return !model_.is_paused() && !model_.is_player_menu_open() && model_.transfer_target() == 0;
}

bool UnitCommand::captures_pointer(ui::Point) const { return pressed_ && dragged_; }

void UnitCommand::update() {
    const input::PointerPosition at = pointer_->position();
    const ::world::IsoProjection projection =
        model_.camera().projection(screen_.width, screen_.height);
    if (pointer_->secondary_released()) {
        model_.order_units(projection.tile_at({at.x, at.y}));
    }
    if (pointer_->down()) {
        const ::world::TileCoord tile = projection.tile_at({at.x, at.y});
        if (!pressed_) {
            // The press fires; holding fires again only with an automatic weapon.
            pressed_ = true;
            dragged_ = false;
            anchor_ = at;
            model_.shoot_at(tile);
        } else if (!dragged_) {
            model_.hold_trigger(tile);
        }
        current_ = at;
        dragged_ = dragged_ || std::hypot(at.x - anchor_.x, at.y - anchor_.y) >
                                   SelectionDragThreshold;
        return;
    }
    if (!pressed_) {
        return;
    }
    const bool finished_drag = dragged_;
    pressed_ = false;
    dragged_ = false;
    // A plain click drops the selection (SiteWork still builds or opens a transfer with it).
    if (!finished_drag) {
        model_.clear_selection();
        return;
    }
    model_.select_units(projection, {anchor_.x, anchor_.y}, {at.x, at.y});
}

void UnitCommand::draw(ui::Canvas& canvas, ui::Size screen) const {
    screen_ = screen;
    if (!pressed_ || !dragged_) {
        return;
    }
    const ui::Rect box{std::min(anchor_.x, current_.x), std::min(anchor_.y, current_.y),
                       std::abs(current_.x - anchor_.x), std::abs(current_.y - anchor_.y)};
    canvas.fill_rect(box, BoxFill);
    canvas.stroke_rect(box, BoxEdge, BoxEdgeWidth);
}

} // namespace scene::world
