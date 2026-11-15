#pragma once

namespace world {

// Kinds of item that can be stored in an inventory.
enum class ItemType { IronOre, CopperOre, Coal };

// An item definition: its type and how many fit in one inventory slot.
struct Item {
    ItemType type;
    int max_stack;

    bool operator==(const Item&) const = default;
};

// The item definitions, one place to tune them.
namespace items {
inline constexpr Item IronOre{ItemType::IronOre, 100};
inline constexpr Item CopperOre{ItemType::CopperOre, 100};
inline constexpr Item Coal{ItemType::Coal, 100};
} // namespace items

// The definition of an item type.
constexpr Item item_of(ItemType type) {
    switch (type) {
        case ItemType::CopperOre: return items::CopperOre;
        case ItemType::Coal: return items::Coal;
        case ItemType::IronOre: break;
    }
    return items::IronOre;
}

// A quantity of one item, as held in a single inventory slot.
struct ItemStack {
    Item item;
    int count;
};

// Name of an item type for the player to read ("Iron Ore", "Coal").
constexpr const char* to_string(ItemType type) {
    switch (type) {
        case ItemType::CopperOre: return "Copper Ore";
        case ItemType::Coal: return "Coal";
        case ItemType::IronOre: break;
    }
    return "Iron Ore";
}

} // namespace world
