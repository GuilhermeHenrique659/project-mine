#include "scene/world/core/components/inventory.hpp"

#include <algorithm>

namespace world {

Inventory::Inventory(std::size_t capacity) : slots_(capacity) {}

std::size_t Inventory::capacity() const { return slots_.size(); }

const std::vector<std::optional<ItemStack>>& Inventory::slots() const { return slots_; }

int Inventory::add(Item item, int count) {
    if (count <= 0) return 0;
    if (item.max_stack <= 0) return count;

    for (auto& slot : slots_) {
        if (count == 0) return 0;
        if (!slot || slot->item.type != item.type) continue;
        const int moved = std::min(count, std::max(0, item.max_stack - slot->count));
        slot->count += moved;
        count -= moved;
    }
    for (auto& slot : slots_) {
        if (count == 0) return 0;
        if (slot) continue;
        const int moved = std::min(count, item.max_stack);
        slot = ItemStack{item, moved};
        count -= moved;
    }
    return count;
}

int Inventory::remove(Item item, int count) {
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

int Inventory::count(Item item) const {
    int total = 0;
    for (const auto& slot : slots_) {
        if (slot && slot->item.type == item.type) total += slot->count;
    }
    return total;
}

void Inventory::set_slot(std::size_t index, std::optional<ItemStack> stack) {
    if (index >= slots_.size()) {
        return;
    }
    slots_[index] = stack;
}

std::size_t Inventory::free_slots() const {
    return static_cast<std::size_t>(
        std::count_if(slots_.begin(), slots_.end(), [](const auto& slot) { return !slot; }));
}

bool Inventory::is_empty() const { return free_slots() == slots_.size(); }

} // namespace world
