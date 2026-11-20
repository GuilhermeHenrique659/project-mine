#pragma once

#include "scene/world/core/items/item.hpp"

namespace world {

// What an inventory stores: Solid holds stacks of solid items in slots (SolidInventory),
// Liquid holds a volume of a single liquid (LiquidInventory).
enum class InventoryType { Solid, Liquid };

// The operations shared by every kind of inventory.
class Inventory {
public:
    virtual ~Inventory() = default;

    virtual InventoryType type() const = 0;

    // Adds up to count of the item. Returns the leftover that did not fit or is not
    // accepted (0 = all added).
    virtual int add(Item item, int count) = 0;

    // Returns how many were removed.
    virtual int remove(Item item, int count) = 0;

    virtual int count(Item item) const = 0;
    virtual bool is_empty() const = 0;

protected:
    Inventory() = default;
    Inventory(const Inventory&) = default;
    Inventory& operator=(const Inventory&) = default;
};

// Moves up to `count` of `item` from one inventory to the other: never more than `from`
// holds nor more than `to` accepts (what does not fit stays in `from`). Returns how many
// were moved.
int transfer_items(Inventory& from, Inventory& to, Item item, int count);

} // namespace world
