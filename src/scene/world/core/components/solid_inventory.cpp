#include "scene/world/core/components/solid_inventory.hpp"

#include <algorithm>

namespace world {

SolidInventory::SolidInventory(std::size_t capacity) : slots_(capacity), filters_(capacity) {}

std::size_t SolidInventory::capacity() const { return slots_.size(); }

const std::vector<std::optional<ItemStack>>& SolidInventory::slots() const { return slots_; }

int SolidInventory::add(Item item, int count) {
    if (count <= 0) return 0;
    if (item.max_stack <= 0 || item.category != ItemCategory::Solid) return count;

    for (std::size_t i = 0; i < slots_.size(); ++i) {
        auto& slot = slots_[i];
        if (count == 0) return 0;
        if (!slot || slot->item.type != item.type || !accepts(i, item.type)) continue;
        const int moved = std::min(count, std::max(0, item.max_stack - slot->count));
        slot->count += moved;
        count -= moved;
    }
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        auto& slot = slots_[i];
        if (count == 0) return 0;
        if (slot || !accepts(i, item.type)) continue;
        const int moved = std::min(count, item.max_stack);
        slot = ItemStack{item, moved};
        count -= moved;
    }
    return count;
}

int SolidInventory::remove(Item item, int count) {
    if (count <= 0) return 0;

    int removed = 0;
    for (auto slot = slots_.rbegin(); slot != slots_.rend() && removed < count; ++slot) {
        if (!*slot || (*slot)->item.type != item.type) continue;
        const int taken = std::min(count - removed, (*slot)->count);
        (*slot)->count -= taken;
        removed += taken;
        if ((*slot)->count == 0) slot->reset();
    }
    return removed;
}

int SolidInventory::count(Item item) const {
    int total = 0;
    for (const auto& slot : slots_) {
        if (slot && slot->item.type == item.type) total += slot->count;
    }
    return total;
}

void SolidInventory::set_slot(std::size_t index, std::optional<ItemStack> stack) {
    if (index >= slots_.size()) {
        return;
    }
    if (stack && (stack->item.category != ItemCategory::Solid ||
                  !accepts(index, stack->item.type))) {
        return;
    }
    slots_[index] = stack;
}

SolidInventory& SolidInventory::append(std::size_t count, ItemType filter) {
    slots_.resize(slots_.size() + count);
    filters_.resize(filters_.size() + count, filter);
    return *this;
}

std::optional<ItemType> SolidInventory::filter(std::size_t index) const {
    if (index >= filters_.size()) {
        return std::nullopt;
    }
    return filters_[index];
}

bool SolidInventory::accepts(std::size_t index, ItemType type) const {
    if (filters_[index]) {
        return *filters_[index] == type;
    }
    // A general slot takes any item that has no filtered slot of its own here.
    return std::find(filters_.begin(), filters_.end(), std::optional<ItemType>{type}) ==
           filters_.end();
}

std::size_t SolidInventory::free_slots() const {
    return static_cast<std::size_t>(
        std::count_if(slots_.begin(), slots_.end(), [](const auto& slot) { return !slot; }));
}

bool SolidInventory::is_empty() const { return free_slots() == slots_.size(); }

int transfer_items(Inventory& from, Inventory& to, Item item, int count) {
    const int taken = from.remove(item, count);
    const int leftover = to.add(item, taken);
    // What did not fit goes back where it came from (it was just taken, so it fits).
    from.add(item, leftover);
    return taken - leftover;
}

} // namespace world
