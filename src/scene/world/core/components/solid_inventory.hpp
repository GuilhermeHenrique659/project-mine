#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

// A fixed number of slots, each holding one stack of a single item, capped by the
// item's max_stack. Attach it to any entity. Stacks never merge or reorder on their own.
// Slots can be filtered (see append()): a filtered slot accepts only its item, and an
// item that has filtered slots here goes only to those, never to the general slots.
class SolidInventory final : public Inventory {
public:
    explicit SolidInventory(std::size_t capacity);

    InventoryType type() const override { return InventoryType::Solid; }

    std::size_t capacity() const;
    const std::vector<std::optional<ItemStack>>& slots() const;

    // Adds up to count items into the slots that accept it: tops up stacks of the same
    // item first, then fills empty slots in order. Returns the leftover that did not fit (0 = all added).
    int add(Item item, int count) override;

    // Takes from the last slots holding the item first. Returns how many were removed.
    int remove(Item item, int count) override;

    // Total of the item across all slots.
    int count(Item item) const override;

    // Puts exactly this stack (or nothing) in one slot, replacing what was there, e.g. to
    // restore a save. An index outside the capacity, or a stack the slot does not accept,
    // is ignored.
    void set_slot(std::size_t index, std::optional<ItemStack> stack);

    // Appends `count` slots that accept only `filter` (a filtered section, e.g. a
    // building's input). Returns *this so a blueprint can chain it.
    SolidInventory& append(std::size_t count, ItemType filter);

    // The only item type slot `index` accepts; empty = any item (a general slot).
    std::optional<ItemType> filter(std::size_t index) const;

    std::size_t free_slots() const;
    bool is_empty() const override;

private:
    bool accepts(std::size_t index, ItemType type) const;

    std::vector<std::optional<ItemStack>> slots_;
    std::vector<std::optional<ItemType>> filters_;
};

} // namespace world
