#pragma once

#include <optional>

#include "scene/world/core/components/inventory.hpp"

namespace world {

// A tank: holds up to `capacity` units of one liquid at a time. Solid items are refused,
// and so is any other liquid until the tank is emptied.
class LiquidInventory final : public Inventory {
public:
    explicit LiquidInventory(int capacity);

    InventoryType type() const override { return InventoryType::Liquid; }
    int add(Item item, int count) override;
    int remove(Item item, int count) override;
    int count(Item item) const override;
    bool is_empty() const override;

    int capacity() const;
    // The liquid held; empty when the tank is empty.
    std::optional<ItemType> liquid() const;

private:
    int capacity_;
    std::optional<Item> liquid_;
    int amount_ = 0;
};

} // namespace world
