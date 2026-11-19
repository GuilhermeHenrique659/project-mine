#include "scene/world/render/player_hud.hpp"

#include <algorithm>

#include "scene/world/core/occupancy.hpp"
#include "scene/world/render/cube.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float TabSize = 64.0f;
constexpr float Padding = 8.0f;
constexpr float Gap = 8.0f;
constexpr float ItemHeight = 56.0f;
constexpr float ItemWidth = 240.0f;
constexpr float BorderWidth = 2.0f;
constexpr ui::Color PreviewValidFront{60, 170, 80, 140};
constexpr ui::Color PreviewValidTop{110, 220, 130, 140};
constexpr ui::Color PreviewBlockedFront{190, 50, 50, 140};
constexpr ui::Color PreviewBlockedTop{235, 100, 100, 140};
// After a refused click: opaque, saturated red.
constexpr ui::Color PreviewRejectedFront{220, 0, 0, 230};
constexpr ui::Color PreviewRejectedTop{255, 40, 40, 230};

const std::vector<::world::ConstructionGroup>& catalog() {
    return ::world::construction_catalog();
}
} // namespace

PlayerHud::PlayerHud(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model), pointer_(std::move(pointer)),
      pick_([this](const ::world::Construction& construction) {
          selected_ = &construction;
          rejected_tile_.reset();
          open_tab_.reset();
      }) {
    ui::BoxBuilder bar = ui::row();
    bar.padding(Padding)
        .gap(Gap)
        .justify(ui::Justify::Center)
        .align(ui::Align::Center)
        .background(ui::theme::Panel)
        .border(ui::theme::Outline, BorderWidth);
    for (std::size_t i = 0; i < catalog().size(); ++i) {
        auto tab = std::make_unique<IconButton>(catalog()[i].icon, "", ui::Size{TabSize, TabSize});
        // Clicking a tab opens its list; clicking the open one closes it.
        tab->set_on_click([this, i] {
            if (open_tab_ == i) {
                open_tab_.reset();
            } else {
                open_tab_ = i;
            }
        });
        bar.child_ref(*tab);
        tabs_.push_back(std::move(tab));
    }
    bar_root_ = bar.build();
}

bool PlayerHud::visible() const { return model_.is_build_bar_open() && !model_.is_paused(); }

bool PlayerHud::captures_pointer(ui::Point position) const {
    return bar_.contains(position) || list_.contains(position);
}

std::size_t PlayerHud::tab_count() const { return tabs_.size(); }

ui::Rect PlayerHud::tab_bounds(std::size_t index) const {
    return index < tabs_.size() ? tabs_[index]->bounds() : ui::Rect{};
}

std::size_t PlayerHud::item_count() const {
    return open_tab_ ? catalog()[*open_tab_].constructions.size() : 0;
}

ui::Rect PlayerHud::item_bounds(std::size_t index) const {
    return index < items_.size() ? items_[index]->bounds() : ui::Rect{};
}

std::unique_ptr<ui::Box> PlayerHud::build_list(std::size_t tab) const {
    ui::BoxBuilder list = ui::column();
    list.padding(Padding).gap(Gap).background(ui::theme::Panel).border(ui::theme::Outline,
                                                                        BorderWidth);
    items_.clear();
    for (const ::world::Construction& construction : catalog()[tab].constructions) {
        auto item = std::make_unique<IconButton>(construction.icon, construction.name,
                                                 ui::Size{ItemWidth, ItemHeight});
        // Picking an entry selects it and closes the list (rebuilt by the next layout).
        item->set_on_click([this, &construction] { pick_(construction); });
        items_.push_back(item.get());
        list.child(std::move(item));
    }
    return list.build();
}

void PlayerHud::update() {
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    const bool released = pointer_->released();
    // The list first (it sits above the bar), then the bar; both from the last layout.
    const bool on_hud = bar_.contains(position) || list_.contains(position);
    if (list_root_) {
        list_root_->handle_pointer(position, released);
    }
    bar_root_->handle_pointer(position, released);
    if (!released || on_hud || selected_ == nullptr) {
        return;
    }
    // A click on the world places the selected construction on the tile under the pointer;
    // a refused one keeps the selection and marks the tile rejected.
    const Preview shown = *preview();
    if (!shown.valid) {
        rejected_tile_ = shown.tile;
        return;
    }
    model_.request_build(*selected_->type, shown.tile);
    selected_ = nullptr;
    rejected_tile_.reset();
}

std::optional<PlayerHud::Preview> PlayerHud::preview() const {
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    if (selected_ == nullptr || bar_.contains(position) || list_.contains(position)) {
        return std::nullopt;
    }
    const ::world::TileCoord tile =
        model_.camera().projection(screen_.width, screen_.height).tile_at({at.x, at.y});
    return Preview{tile, model_.placement_status(*selected_, tile) == ::world::BuildResult::Ok,
                   rejected_tile_ == tile};
}

void PlayerHud::draw_preview(ui::Canvas& canvas) const {
    const auto shown = preview();
    if (!shown) {
        return;
    }
    const ::world::IsoProjection projection =
        model_.camera().projection(screen_.width, screen_.height);
    // The preview covers the selection's whole footprint, from the tile under the pointer.
    const ::world::TileSize footprint = selected_->size;
    const ::world::ScreenPoint center =
        projection.world_to_screen(::world::footprint_centre(shown->tile, footprint));
    const float tile = BuildingCubeSize * projection.tile_width / ::world::IsoProjection{}.tile_width;
    // Translucent green where it can be built, red where it cannot.
    const Cube cube =
        shown->valid      ? Cube::footprint(tile, footprint, PreviewValidFront, PreviewValidTop)
        : shown->rejected ? Cube::footprint(tile, footprint, PreviewRejectedFront, PreviewRejectedTop)
                          : Cube::footprint(tile, footprint, PreviewBlockedFront, PreviewBlockedTop);
    cube.draw(canvas, center);
}

bool PlayerHud::is_disabled(const ::world::Construction& construction) const {
    return model_.build_status(construction) != ::world::BuildResult::Ok;
}

void PlayerHud::layout(const ui::Canvas& canvas, ui::Size screen) const {
    screen_ = screen;
    // Flush with the bottom edge of the screen, at least BarWidthCoverage wide; the tabs
    // are centred inside it.
    const ui::Size wanted = bar_root_->preferred_size(canvas);
    const float width = std::max(screen.width * BarWidthCoverage, wanted.width);
    bar_root_->layout({(screen.width - width) / 2.0f, screen.height - wanted.height, width,
                       wanted.height},
                      canvas);
    bar_ = bar_root_->bounds();
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        tabs_[i]->set_active(open_tab_ == i);
    }

    if (list_tab_ != open_tab_) {
        list_root_ = open_tab_ ? build_list(*open_tab_) : nullptr;
        if (!open_tab_) {
            items_.clear();
        }
        list_tab_ = open_tab_;
    }
    list_ = {};
    if (!list_root_) {
        return;
    }
    // Constructions the model refuses now are disabled.
    const auto& constructions = catalog()[*open_tab_].constructions;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        items_[i]->set_enabled(!is_disabled(constructions[i]));
    }
    // The drop-up grows upwards from the top of the bar, aligned with the left of its tab.
    const ui::Size size = list_root_->preferred_size(canvas);
    const ui::Rect& tab = tabs_[*open_tab_]->bounds();
    list_root_->layout({tab.x - Padding, bar_.y - size.height, size.width, size.height}, canvas);
    list_ = list_root_->bounds();
}

void PlayerHud::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    draw_preview(canvas);
    bar_root_->draw(canvas);
    if (list_root_) {
        list_root_->draw(canvas);
    }
}

} // namespace scene::world
