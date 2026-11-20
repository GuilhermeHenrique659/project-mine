#include "scene/world/core/components/liquid_inventory.hpp"

#include <algorithm>

namespace world {

LiquidInventory::LiquidInventory(int capacity) : capacity_(std::max(0, capacity)) {}

int LiquidInventory::add(Item item, int count) {
    if (count <= 0) return 0;
    if (item.category != ItemCategory::Liquid) return count;
    if (liquid_ && liquid_->type != item.type) return count;

    const int added = std::min(count, capacity_ - amount_);
    if (added == 0) return count;
    liquid_ = item;
    amount_ += added;
    return count - added;
}

int LiquidInventory::remove(Item item, int count) {
    if (count <= 0 || !liquid_ || liquid_->type != item.type) return 0;

    const int removed = std::min(count, amount_);
    amount_ -= removed;
    if (amount_ == 0) liquid_.reset();
    return removed;
}

int LiquidInventory::count(Item item) const {
    if (!liquid_ || liquid_->type != item.type) return 0;
    return amount_;
}

bool LiquidInventory::is_empty() const { return amount_ == 0; }

int LiquidInventory::capacity() const { return capacity_; }

std::optional<ItemType> LiquidInventory::liquid() const {
    if (!liquid_) return std::nullopt;
    return liquid_->type;
}

} // namespace world
