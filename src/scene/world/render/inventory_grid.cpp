#include "scene/world/render/inventory_grid.hpp"

#include <algorithm>
#include <string>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/render/terrain_renderer.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
// Space between a slot's edge and the item drawn in it.
constexpr float ItemInset = 8.0f;
constexpr int CountFontSize = 18;
// Space between the count and the slot's bottom-right corner.
constexpr float CountMargin = 4.0f;
} // namespace

ui::Color item_color(::world::ItemType type) {
    switch (type) {
    case ::world::ItemType::CopperOre: return CopperOreColor;
    case ::world::ItemType::Coal: return CoalColor;
    case ::world::ItemType::IronOre: break;
    }
    return IronOreColor;
}

InventoryGrid::InventoryGrid(const core::Registry& registry, core::EntityId entity)
    : ui::Widget(ui::theme::slot_style()), registry_(registry), entity_(entity) {}

std::size_t InventoryGrid::capacity() const {
    const auto* inventory = registry_.get<::world::Inventory>(entity_);
    return inventory == nullptr ? 0 : inventory->capacity();
}

ui::Size InventoryGrid::preferred_size(const ui::Canvas&) const {
    const std::size_t slots = capacity();
    if (slots == 0) {
        return {};
    }
    const std::size_t columns = std::min<std::size_t>(slots, Columns);
    const std::size_t rows = (slots + Columns - 1) / Columns;
    return {static_cast<float>(columns) * SlotSize + static_cast<float>(columns - 1) * SlotGap,
            static_cast<float>(rows) * SlotSize + static_cast<float>(rows - 1) * SlotGap};
}

void InventoryGrid::layout(ui::Rect bounds, const ui::Canvas& canvas) {
    const ui::Size size = preferred_size(canvas);
    bounds_ = {bounds.x, bounds.y, size.width, size.height};
    slot_bounds_.clear();
    const std::size_t slots = capacity();
    for (std::size_t i = 0; i < slots; ++i) {
        const auto column = static_cast<float>(i % Columns);
        const auto row = static_cast<float>(i / Columns);
        slot_bounds_.push_back({bounds.x + column * (SlotSize + SlotGap),
                                bounds.y + row * (SlotSize + SlotGap), SlotSize, SlotSize});
    }
}

void InventoryGrid::handle_pointer(ui::Point position, bool released) {
    ui::Widget::handle_pointer(position, released);
    hovered_slot_ = NoSlot;
    for (std::size_t i = 0; i < slot_bounds_.size(); ++i) {
        if (slot_bounds_[i].contains(position)) {
            hovered_slot_ = i;
            return;
        }
    }
}

void InventoryGrid::draw(ui::Canvas& canvas) const {
    const auto* inventory = registry_.get<::world::Inventory>(entity_);
    if (inventory == nullptr) {
        return;
    }
    const auto& slots = inventory->slots();
    for (std::size_t i = 0; i < slot_bounds_.size() && i < slots.size(); ++i) {
        const ui::Rect& slot = slot_bounds_[i];
        const ui::Color face = i == hovered_slot_
                                   ? style_.hover_background.value_or(style_.background)
                                   : style_.background;
        ui::draw_raised(canvas, slot, face, style_);
        if (!slots[i]) {
            continue;
        }
        canvas.fill_rect({slot.x + ItemInset, slot.y + ItemInset, slot.width - 2.0f * ItemInset,
                          slot.height - 2.0f * ItemInset},
                         item_color(slots[i]->item.type));
        const std::string count = std::to_string(slots[i]->count);
        const ui::Size text = canvas.measure_text(count, CountFontSize);
        canvas.draw_text(count,
                         {slot.x + slot.width - text.width - CountMargin,
                          slot.y + slot.height - text.height - CountMargin},
                         CountFontSize, ui::theme::Text);
    }
}

} // namespace scene::world
