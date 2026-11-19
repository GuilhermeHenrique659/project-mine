#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/items/item.hpp"
#include "ui/composite.hpp"

namespace scene::world {

// The colour an item is drawn with in an inventory: the colour of its ore on the ground.
ui::Color item_color(::world::ItemType type);

// One slot of an inventory: a square button-like face (theme::slot_style(), darker while
// hovered); a filled slot shows the item's colour and its count in the bottom-right corner,
// an empty filtered one its item faintly. It reads the slot from the registry on every draw.
class InventorySlot : public ui::Widget {
public:
    InventorySlot(const core::Registry& registry, core::EntityId entity, std::size_t index);

    ui::Size preferred_size(const ui::Canvas& canvas) const override;
    void draw(ui::Canvas& canvas) const override;

private:
    const core::Registry& registry_;
    core::EntityId entity_;
    std::size_t index_;
};

// Draws an entity's inventory as a ui::grid() of InventorySlots, Columns per row. It reads
// the inventory from the registry on every layout/draw, so it always shows the current
// contents; an entity without an inventory shows no slots. Clicking a slot calls
// on_slot_click(index). It always lays out every row: put it in a ui::ScrollBox to show a
// few rows of a big one (step: RowStep). The slots are rebuilt only when the capacity
// changes. The registry must outlive it.
class InventoryGrid : public ui::Composite {
public:
    static constexpr int Columns = 10;
    static constexpr float SlotSize = 64.0f;
    static constexpr float SlotGap = 6.0f;
    // Height of a row with the gap below it: the scroll step of a grid in a ScrollBox.
    static constexpr float RowStep = SlotSize + SlotGap;

    using SlotHandler = std::function<void(std::size_t slot)>;

    InventoryGrid(const core::Registry& registry, core::EntityId entity);

    void set_on_slot_click(SlotHandler handler) { on_slot_click_ = std::move(handler); }

    // Screen bounds of each slot, from the last layout().
    std::vector<ui::Rect> slot_bounds() const;

private:
    void sync(const ui::Canvas& canvas) const override;
    std::size_t capacity() const;

    const core::Registry& registry_;
    core::EntityId entity_;
    SlotHandler on_slot_click_;
    mutable std::vector<InventorySlot*> slots_; // owned by the tree
    mutable bool built_ = false;
};

} // namespace scene::world
