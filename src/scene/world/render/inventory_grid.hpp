#pragma once

#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/items/item.hpp"
#include "ui/widget.hpp"

namespace scene::world {

// The colour an item is drawn with in an inventory: the colour of its ore on the ground.
ui::Color item_color(::world::ItemType type);

// Draws an entity's inventory as rows of square slots, Columns per row. A filled slot
// shows the item's colour and its count in the bottom-right corner. It reads the
// inventory from the registry on every layout/draw, so it always shows the current
// contents; an entity without an inventory shows no slots. The registry must outlive it.
class InventoryGrid : public ui::Widget {
public:
    static constexpr int Columns = 10;
    static constexpr float SlotSize = 64.0f;
    static constexpr float SlotGap = 6.0f;

    InventoryGrid(const core::Registry& registry, core::EntityId entity);

    ui::Size preferred_size(const ui::Canvas& canvas) const override;
    void layout(ui::Rect bounds, const ui::Canvas& canvas) override;
    void draw(ui::Canvas& canvas) const override;
    // Tracks the slot under the pointer, drawn in the hover colour.
    void handle_pointer(ui::Point position, bool released) override;

    // Screen bounds of each slot, from the last layout().
    const std::vector<ui::Rect>& slot_bounds() const { return slot_bounds_; }

private:
    std::size_t capacity() const;

    const core::Registry& registry_;
    core::EntityId entity_;
    std::vector<ui::Rect> slot_bounds_;
    static constexpr std::size_t NoSlot = static_cast<std::size_t>(-1);
    // The slot under the pointer, or NoSlot.
    std::size_t hovered_slot_ = NoSlot;
};

} // namespace scene::world
