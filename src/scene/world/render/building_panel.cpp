#include "scene/world/render/building_panel.hpp"

#include <algorithm>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/render/building_details.hpp"
#include "scene/world/render/entity_name.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float Padding = 24.0f;
constexpr float TitleGap = 8.0f;
constexpr float SectionGap = 24.0f;
constexpr float BorderWidth = 2.0f;
constexpr int TitleFontSize = 20;
constexpr int DetailFontSize = 16;
constexpr float DetailGap = 4.0f;
// Rows of a building's inventory shown at once; a bigger one (a storage) scrolls.
constexpr std::size_t BuildingVisibleRows = 4;

ui::Rect slot_or_empty(const InventoryGrid& grid, std::size_t index) {
    const auto& slots = grid.slot_bounds();
    return index < slots.size() ? slots[index] : ui::Rect{};
}
} // namespace

BuildingPanel::BuildingPanel(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model),
      pointer_(std::move(pointer)),
      player_grid_(std::make_unique<InventoryGrid>(model.registry(), model.player_id())),
      close_button_(std::make_unique<ui::Button>("X", ui::theme::close_button_style(), true)) {
    close_button_->set_on_click([&model] { model.close_transfer(); });
}

bool BuildingPanel::visible() const { return model_.transfer_target() != 0 && !model_.is_paused(); }

InventoryGrid& BuildingPanel::building_grid() const {
    const core::EntityId target = model_.transfer_target();
    if (!building_grid_ || building_ != target) {
        building_grid_ = std::make_unique<InventoryGrid>(model_.registry(), target, BuildingVisibleRows);
        building_ = target;
    }
    return *building_grid_;
}

ui::Rect BuildingPanel::player_slot_bounds(std::size_t index) const {
    return slot_or_empty(*player_grid_, index);
}

ui::Rect BuildingPanel::building_slot_bounds(std::size_t index) const {
    return slot_or_empty(building_grid(), index);
}

void BuildingPanel::move_stack(core::EntityId from, core::EntityId to, std::size_t slot) const {
    const auto* inventory = model_.registry().get<::world::Inventory>(from);
    if (inventory == nullptr || slot >= inventory->capacity() || !inventory->slots()[slot]) {
        return;
    }
    const ::world::ItemStack stack = *inventory->slots()[slot];
    model_.request_transfer(from, to, stack.item, stack.count);
}

void BuildingPanel::update() {
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    const bool released = pointer_->released();
    player_grid_->handle_pointer(position, released);
    building_grid().handle_pointer(position, released);
    if (building_grid().bounds().contains(position)) {
        building_grid().scroll(pointer_->scroll());
    }
    if (!released) {
        return;
    }
    const core::EntityId player = model_.player_id();
    const core::EntityId building = model_.transfer_target();
    const auto& player_slots = player_grid_->slot_bounds();
    for (std::size_t i = 0; i < player_slots.size(); ++i) {
        if (player_slots[i].contains(position)) {
            move_stack(player, building, i);
            return;
        }
    }
    const auto& building_slots = building_grid().slot_bounds();
    for (std::size_t i = 0; i < building_slots.size(); ++i) {
        if (building_slots[i].contains(position)) {
            move_stack(building, player, i);
            return;
        }
    }
    // Last: a click on it closes the panel.
    close_button_->handle_pointer(position, released);
}

void BuildingPanel::layout(const ui::Canvas& canvas, ui::Size screen) const {
    InventoryGrid& building = building_grid();
    const ui::Size player_size = player_grid_->preferred_size(canvas);
    const ui::Size building_size = building.preferred_size(canvas);
    const ui::Size close = close_button_->preferred_size(canvas);
    const float title = static_cast<float>(TitleFontSize);
    details_ = building_details(model_.registry(), model_.transfer_target());
    const float detail_line = static_cast<float>(DetailFontSize) + DetailGap;
    const float details_height = static_cast<float>(details_.size()) * detail_line;
    const float width =
        std::max(player_size.width, building_size.width) + 2.0f * Padding;
    const float height = 2.0f * Padding + std::max(title, close.height) + TitleGap +
                         player_size.height + SectionGap + title + TitleGap + details_height +
                         building_size.height;
    panel_ = {(screen.width - width) / 2.0f, (screen.height - height) / 2.0f, width, height};

    close_button_->layout({panel_.x + width - Padding - close.width, panel_.y + Padding,
                           close.width, close.height},
                          canvas);
    float y = panel_.y + Padding;
    player_title_ = {panel_.x + Padding, y};
    y += std::max(title, close.height) + TitleGap;
    player_grid_->layout({panel_.x + Padding, y, player_size.width, player_size.height}, canvas);
    y += player_size.height + SectionGap;
    building_title_ = {panel_.x + Padding, y};
    y += title + TitleGap;
    details_top_ = y;
    y += details_height;
    building.layout({panel_.x + Padding, y, building_size.width, building_size.height}, canvas);
}

void BuildingPanel::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    canvas.fill_rect(panel_, ui::theme::Panel);
    canvas.stroke_rect(panel_, ui::theme::Outline, BorderWidth);
    canvas.draw_text("Player", player_title_, TitleFontSize, ui::theme::Text);
    const core::Entity* building = model_.registry().find(model_.transfer_target());
    canvas.draw_text(building != nullptr ? entity_name(building->label, building->id) : "",
                     building_title_, TitleFontSize,
                     ui::theme::Text);
    for (std::size_t i = 0; i < details_.size(); ++i) {
        const float line = static_cast<float>(DetailFontSize) + DetailGap;
        canvas.draw_text(details_[i],
                         {building_title_.x, details_top_ + static_cast<float>(i) * line},
                         DetailFontSize, ui::theme::Text);
    }
    player_grid_->draw(canvas);
    building_grid().draw(canvas);
    close_button_->draw(canvas);
}

} // namespace scene::world
