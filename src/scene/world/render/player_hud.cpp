#include "scene/world/render/player_hud.hpp"

#include <algorithm>

#include "graphics/sprite.hpp"
#include "scene/world/render/cube.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float TabSize = 64.0f;
constexpr float IconInset = 8.0f;
constexpr float Padding = 8.0f;
constexpr float Gap = 8.0f;
constexpr float ItemHeight = 56.0f;
constexpr float ItemWidth = 240.0f;
constexpr float BorderWidth = 2.0f;
constexpr int FontSize = 20;
constexpr ui::Color PreviewValidFront{60, 170, 80, 140};
constexpr ui::Color PreviewValidTop{110, 220, 130, 140};
constexpr ui::Color PreviewBlockedFront{190, 50, 50, 140};
constexpr ui::Color PreviewBlockedTop{235, 100, 100, 140};

const std::vector<::world::ConstructionGroup>& catalog() {
    return ::world::construction_catalog();
}

ui::Rect inset(const ui::Rect& rect, float by) {
    return {rect.x + by, rect.y + by, rect.width - 2.0f * by, rect.height - 2.0f * by};
}
} // namespace

PlayerHud::PlayerHud(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model), pointer_(std::move(pointer)) {}

bool PlayerHud::visible() const { return model_.is_build_bar_open() && !model_.is_paused(); }

std::size_t PlayerHud::tab_count() const { return catalog().size(); }

ui::Rect PlayerHud::tab_bounds(std::size_t index) const {
    return index < tabs_.size() ? tabs_[index] : ui::Rect{};
}

std::size_t PlayerHud::item_count() const {
    return open_tab_ ? catalog()[*open_tab_].constructions.size() : 0;
}

ui::Rect PlayerHud::item_bounds(std::size_t index) const {
    return index < items_.size() ? items_[index] : ui::Rect{};
}

void PlayerHud::update() {
    if (!pointer_->released()) {
        return;
    }
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (!items_[i].contains(position)) {
            continue;
        }
        const ::world::Construction& construction = catalog()[*open_tab_].constructions[i];
        if (is_disabled(construction)) {
            return;
        }
        selected_ = &construction;
        open_tab_.reset();
        return;
    }
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        if (!tabs_[i].contains(position)) {
            continue;
        }
        if (open_tab_ == i) {
            open_tab_.reset();
        } else {
            open_tab_ = i;
        }
        return;
    }
    if (selected_ == nullptr || bar_.contains(position) || list_.contains(position)) {
        return;
    }
    // A click on the world places the selected construction on the tile under the pointer.
    model_.request_build(*selected_->type, preview()->tile);
    selected_ = nullptr;
}

std::optional<PlayerHud::Preview> PlayerHud::preview() const {
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    if (selected_ == nullptr || bar_.contains(position) || list_.contains(position)) {
        return std::nullopt;
    }
    const ::world::TileCoord tile =
        model_.camera().projection(screen_.width, screen_.height).tile_at({at.x, at.y});
    return Preview{tile, model_.placement_status(*selected_, tile) == ::world::BuildResult::Ok};
}

void PlayerHud::draw_preview(ui::Canvas& canvas) const {
    const auto shown = preview();
    if (!shown) {
        return;
    }
    const ::world::IsoProjection projection =
        model_.camera().projection(screen_.width, screen_.height);
    const ::world::ScreenPoint center = projection.world_to_screen(
        {shown->tile.x + 0.5f, shown->tile.y + 0.5f});
    const float size = BuildingCubeSize * projection.tile_width / ::world::IsoProjection{}.tile_width;
    // Translucent green where it can be built, red where it cannot.
    const Cube cube = shown->valid ? Cube(size, PreviewValidFront, PreviewValidTop)
                                   : Cube(size, PreviewBlockedFront, PreviewBlockedTop);
    cube.draw(canvas, center);
}

bool PlayerHud::is_disabled(const ::world::Construction& construction) const {
    return model_.build_status(construction) != ::world::BuildResult::Ok;
}

void PlayerHud::layout(const ui::Canvas&, ui::Size screen) const {
    screen_ = screen;
    const std::size_t count = tab_count();
    const float tabs_width = count * TabSize + (count > 0 ? (count - 1) * Gap : 0.0f);
    const float width = std::max(screen.width * BarWidthCoverage, tabs_width + 2.0f * Padding);
    const float height = 2.0f * Padding + TabSize;
    // Flush with the bottom edge of the screen; the tabs are centred inside it.
    bar_ = {(screen.width - width) / 2.0f, screen.height - height, width, height};

    tabs_.clear();
    const float tabs_x = bar_.x + (width - tabs_width) / 2.0f;
    for (std::size_t i = 0; i < count; ++i) {
        tabs_.push_back({tabs_x + i * (TabSize + Gap), bar_.y + Padding, TabSize, TabSize});
    }

    items_.clear();
    list_ = {};
    if (!open_tab_) {
        return;
    }
    // The drop-up grows upwards from the top of the bar, aligned with the left of its tab.
    const std::size_t items = item_count();
    const float list_height = 2.0f * Padding + items * ItemHeight + (items - 1) * Gap;
    const ui::Rect& tab = tabs_[*open_tab_];
    list_ = {tab.x - Padding, bar_.y - list_height, ItemWidth + 2.0f * Padding, list_height};
    for (std::size_t i = 0; i < items; ++i) {
        items_.push_back({list_.x + Padding, list_.y + Padding + i * (ItemHeight + Gap), ItemWidth,
                          ItemHeight});
    }
}

void PlayerHud::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    const input::PointerPosition at = pointer_->position();
    const ui::Point mouse{at.x, at.y};
    const ui::Style style = ui::theme::slot_style();

    draw_preview(canvas);
    canvas.fill_rect(bar_, ui::theme::Panel);
    canvas.stroke_rect(bar_, ui::theme::Outline, BorderWidth);
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        const ui::Color face = open_tab_ == i           ? ui::theme::GreenFace
                               : tabs_[i].contains(mouse) ? ui::theme::ButtonFaceHover
                                                          : ui::theme::ButtonFace;
        ui::draw_raised(canvas, tabs_[i], face, style);
        graphics::draw_sprite(canvas, {catalog()[i].icon, std::nullopt}, inset(tabs_[i], IconInset));
    }

    if (!open_tab_) {
        return;
    }
    canvas.fill_rect(list_, ui::theme::Panel);
    canvas.stroke_rect(list_, ui::theme::Outline, BorderWidth);
    const auto& constructions = catalog()[*open_tab_].constructions;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const ui::Rect& item = items_[i];
        const bool disabled = is_disabled(constructions[i]);
        const ui::Color face = disabled              ? ui::theme::ButtonShadow
                               : item.contains(mouse) ? ui::theme::ButtonFaceHover
                                                      : ui::theme::ButtonFace;
        ui::draw_raised(canvas, item, face, style);
        const ui::Rect icon{item.x + IconInset, item.y + IconInset, item.height - 2.0f * IconInset,
                            item.height - 2.0f * IconInset};
        graphics::draw_sprite(canvas, {constructions[i].icon, std::nullopt}, icon);
        const float text_y = item.y + (item.height - FontSize) / 2.0f;
        canvas.draw_text(constructions[i].name, {icon.x + icon.width + IconInset, text_y}, FontSize,
                         disabled ? ui::theme::Muted : ui::theme::Text);
    }
}

} // namespace scene::world
