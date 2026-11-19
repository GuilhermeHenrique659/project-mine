#include "scene/world/render/inventory_grid.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/render/terrain_renderer.hpp"
#include "ui/grid.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
// Space between a slot's edge and the item drawn in it.
constexpr float ItemInset = 8.0f;
constexpr int CountFontSize = 18;
// Space between the count and the slot's bottom-right corner.
constexpr float CountMargin = 4.0f;
// Items that are not a tile (ores reuse their tile color).
constexpr ui::Color SteelColor{185, 190, 198, 255};
constexpr ui::Color ElectronicsColor{60, 170, 90, 255};
constexpr ui::Color ToolColor{210, 150, 60, 255};
constexpr ui::Color AmmoColor{200, 175, 70, 255};
constexpr ui::Color WeaponColor{95, 100, 115, 255};
// Opacity of the item hint in an empty filtered slot.
constexpr unsigned char FilterHintAlpha = 60;
} // namespace

ui::Color item_color(::world::ItemType type) {
    switch (type) {
    case ::world::ItemType::CopperOre: return CopperOreColor;
    case ::world::ItemType::Coal: return CoalColor;
    case ::world::ItemType::Steel: return SteelColor;
    case ::world::ItemType::Electronics: return ElectronicsColor;
    case ::world::ItemType::Tool: return ToolColor;
    case ::world::ItemType::Ammo: return AmmoColor;
    case ::world::ItemType::Weapon: return WeaponColor;
    case ::world::ItemType::IronOre: break;
    }
    return IronOreColor;
}

InventorySlot::InventorySlot(const core::Registry& registry, core::EntityId entity,
                             std::size_t index)
    : ui::Widget(ui::theme::slot_style()), registry_(registry), entity_(entity), index_(index) {}

ui::Size InventorySlot::preferred_size(const ui::Canvas&) const {
    return {InventoryGrid::SlotSize, InventoryGrid::SlotSize};
}

void InventorySlot::draw(ui::Canvas& canvas) const {
    const auto* inventory = registry_.get<::world::Inventory>(entity_);
    if (inventory == nullptr || index_ >= inventory->slots().size()) {
        return;
    }
    const ui::Rect& slot = bounds_;
    const ui::Color face =
        hovered() ? style_.hover_background.value_or(style_.background) : style_.background;
    ui::draw_raised(canvas, slot, face, style_);
    const ui::Rect item{slot.x + ItemInset, slot.y + ItemInset, slot.width - 2.0f * ItemInset,
                        slot.height - 2.0f * ItemInset};
    const auto& stack = inventory->slots()[index_];
    if (!stack) {
        // An empty filtered slot shows its item faintly.
        if (const auto filter = inventory->filter(index_)) {
            ui::Color hint = item_color(*filter);
            hint.a = FilterHintAlpha;
            canvas.fill_rect(item, hint);
        }
        return;
    }
    canvas.fill_rect(item, item_color(stack->item.type));
    const std::string count = std::to_string(stack->count);
    const ui::Size text = canvas.measure_text(count, CountFontSize);
    canvas.draw_text(count,
                     {slot.x + slot.width - text.width - CountMargin,
                      slot.y + slot.height - text.height - CountMargin},
                     CountFontSize, ui::theme::Text);
}

InventoryGrid::InventoryGrid(const core::Registry& registry, core::EntityId entity)
    : ui::Composite(ui::theme::slot_style()), registry_(registry), entity_(entity) {}

std::size_t InventoryGrid::capacity() const {
    const auto* inventory = registry_.get<::world::Inventory>(entity_);
    return inventory == nullptr ? 0 : inventory->capacity();
}

void InventoryGrid::sync(const ui::Canvas&) const {
    const std::size_t slots = capacity();
    if (built_ && slots == slots_.size()) {
        return;
    }
    slots_.clear();
    ui::GridBuilder grid = ui::grid(Columns);
    grid.gap(SlotGap);
    for (std::size_t i = 0; i < slots; ++i) {
        auto slot = std::make_unique<InventorySlot>(registry_, entity_, i);
        slot->set_on_click([this, i] {
            if (on_slot_click_) on_slot_click_(i);
        });
        slots_.push_back(slot.get());
        grid.child(std::move(slot));
    }
    set_tree(grid.build());
    built_ = true;
}

std::vector<ui::Rect> InventoryGrid::slot_bounds() const {
    std::vector<ui::Rect> bounds;
    for (const InventorySlot* slot : slots_) {
        bounds.push_back(slot->bounds());
    }
    return bounds;
}

} // namespace scene::world
