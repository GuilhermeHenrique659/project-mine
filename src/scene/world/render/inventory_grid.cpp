#include "scene/world/render/inventory_grid.hpp"

#include <algorithm>
#include <cmath>
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
// Space between the last column and the scrollbar.
constexpr float ScrollbarGap = 8.0f;
} // namespace

ui::Color item_color(::world::ItemType type) {
    switch (type) {
    case ::world::ItemType::CopperOre: return CopperOreColor;
    case ::world::ItemType::Coal: return CoalColor;
    case ::world::ItemType::IronOre: break;
    }
    return IronOreColor;
}

InventoryGrid::InventoryGrid(const core::Registry& registry, core::EntityId entity,
                             std::size_t visible_rows)
    : ui::Widget(ui::theme::slot_style()), registry_(registry), entity_(entity),
      visible_rows_(visible_rows) {}

std::size_t InventoryGrid::capacity() const {
    const auto* inventory = registry_.get<::world::Inventory>(entity_);
    return inventory == nullptr ? 0 : inventory->capacity();
}

std::size_t InventoryGrid::total_rows() const { return (capacity() + Columns - 1) / Columns; }

std::size_t InventoryGrid::shown_rows() const {
    const std::size_t rows = total_rows();
    return visible_rows_ == 0 ? rows : std::min(rows, visible_rows_);
}

std::size_t InventoryGrid::last_first_row() const { return total_rows() - shown_rows(); }

void InventoryGrid::scroll(float notches) {
    const auto rows = static_cast<long long>(std::lround(notches));
    const auto target = static_cast<long long>(first_row_) - rows;
    first_row_ = static_cast<std::size_t>(
        std::clamp<long long>(target, 0, static_cast<long long>(last_first_row())));
}

ui::Size InventoryGrid::preferred_size(const ui::Canvas&) const {
    const std::size_t slots = capacity();
    if (slots == 0) {
        return {};
    }
    const std::size_t columns = std::min<std::size_t>(slots, Columns);
    const std::size_t rows = shown_rows();
    const float scrollbar = last_first_row() > 0 ? ScrollbarGap + ScrollbarWidth : 0.0f;
    return {static_cast<float>(columns) * SlotSize + static_cast<float>(columns - 1) * SlotGap +
                scrollbar,
            static_cast<float>(rows) * SlotSize + static_cast<float>(rows - 1) * SlotGap};
}

void InventoryGrid::layout(ui::Rect bounds, const ui::Canvas& canvas) {
    const ui::Size size = preferred_size(canvas);
    bounds_ = {bounds.x, bounds.y, size.width, size.height};
    slot_bounds_.clear();
    // The inventory may have shrunk since the last scroll.
    first_row_ = std::min(first_row_, last_first_row());
    const std::size_t slots = capacity();
    const std::size_t end_row = first_row_ + shown_rows();
    for (std::size_t i = 0; i < slots; ++i) {
        const std::size_t row = i / Columns;
        if (row < first_row_ || row >= end_row) {
            slot_bounds_.push_back({});
            continue;
        }
        const auto column = static_cast<float>(i % Columns);
        const auto shown_row = static_cast<float>(row - first_row_);
        slot_bounds_.push_back({bounds.x + column * (SlotSize + SlotGap),
                                bounds.y + shown_row * (SlotSize + SlotGap), SlotSize, SlotSize});
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
        if (slot.width <= 0.0f) {
            continue; // scrolled out of view
        }
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
    draw_scrollbar(canvas);
}

void InventoryGrid::draw_scrollbar(ui::Canvas& canvas) const {
    const std::size_t hidden = last_first_row();
    if (hidden == 0) {
        return;
    }
    const ui::Rect track{bounds_.x + bounds_.width - ScrollbarWidth, bounds_.y, ScrollbarWidth,
                         bounds_.height};
    canvas.fill_rect(track, ui::theme::Outline);
    // The thumb's share of the track is the share of rows shown.
    const float share = static_cast<float>(shown_rows()) / static_cast<float>(total_rows());
    const float thumb_height = track.height * share;
    const float offset = (track.height - thumb_height) * static_cast<float>(first_row_) /
                         static_cast<float>(hidden);
    canvas.fill_rect({track.x, track.y + offset, track.width, thumb_height}, ui::theme::Accent);
}

} // namespace scene::world
