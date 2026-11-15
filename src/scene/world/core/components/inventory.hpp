#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "scene/world/core/items/item.hpp"

namespace world {

// A fixed number of slots, each holding one stack of a single item, capped by the
// item's max_stack. Attach it to any entity. Stacks never merge or reorder on their own.
class Inventory {
public:
    explicit Inventory(std::size_t capacity);

    std::size_t capacity() const;
    const std::vector<std::optional<ItemStack>>& slots() const;

    // Adds up to count items: tops up stacks of the same item first, then fills empty
    // slots in order. Returns the leftover that did not fit (0 = all added).
    int add(Item item, int count);

    // Takes from the last slots holding the item first. Returns how many were removed.
    int remove(Item item, int count);

    // Total of the item across all slots.
    int count(Item item) const;

    // Puts exactly this stack (or nothing) in one slot, replacing what was there, e.g. to
    // restore a save. An index outside the capacity is ignored.
    void set_slot(std::size_t index, std::optional<ItemStack> stack);

    std::size_t free_slots() const;
    bool is_empty() const;

private:
    std::vector<std::optional<ItemStack>> slots_;
};

// Moves up to `count` of `item` from one inventory to the other: never more than `from`
// holds nor more than fits in `to` (what does not fit stays in `from`). Returns how many
// were moved.
int transfer_items(Inventory& from, Inventory& to, Item item, int count);

} // namespace world
