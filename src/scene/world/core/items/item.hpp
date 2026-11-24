#pragma once

#include <optional>

namespace world {

// Kinds of item that can be stored in an inventory.
enum class ItemType { IronOre, CopperOre, Coal, Steel, Electronics, Tool, Ammo, Weapon, Oil, Diesel, Engine };

// Physical state of an item: decides which inventories can store it (see InventoryType).
enum class ItemCategory { Solid, Liquid };

// An item definition: its type and how many fit in one inventory slot.
struct Item {
    ItemType type;
    int max_stack;
    ItemCategory category = ItemCategory::Solid;

    bool operator==(const Item&) const = default;
};

// The item definitions, one place to tune them.
namespace items {
inline constexpr Item IronOre{ItemType::IronOre, 100};
inline constexpr Item CopperOre{ItemType::CopperOre, 100};
inline constexpr Item Coal{ItemType::Coal, 100};
inline constexpr Item Steel{ItemType::Steel, 100};
inline constexpr Item Electronics{ItemType::Electronics, 100};
inline constexpr Item Tool{ItemType::Tool, 100};
inline constexpr Item Ammo{ItemType::Ammo, 100};
inline constexpr Item Weapon{ItemType::Weapon, 1};
inline constexpr Item Engine{ItemType::Engine, 100};
inline constexpr Item Oil{ItemType::Oil, 100, ItemCategory::Liquid};
inline constexpr Item Diesel{ItemType::Diesel, 100, ItemCategory::Liquid};
} // namespace items

// The definition of an item type.
constexpr Item item_of(ItemType type) {
    switch (type) {
        case ItemType::CopperOre: return items::CopperOre;
        case ItemType::Coal: return items::Coal;
        case ItemType::Steel: return items::Steel;
        case ItemType::Electronics: return items::Electronics;
        case ItemType::Tool: return items::Tool;
        case ItemType::Ammo: return items::Ammo;
        case ItemType::Weapon: return items::Weapon;
        case ItemType::Oil: return items::Oil;
        case ItemType::Diesel: return items::Diesel;
        case ItemType::Engine: return items::Engine;
        case ItemType::IronOre: break;
    }
    return items::IronOre;
}

// How the player fires a weapon: Automatic keeps firing while the button is held,
// SemiAutomatic fires once per click.
enum class FireMode { SemiAutomatic, Automatic };

// What a weapon item does when fired: `range` in tiles, `damage` in hit points,
// `cooldown` in seconds between shots, its fire mode and `accuracy`, the chance (0 to 1) a
// shot by a shooter without experience hits (see hit_chance).
struct WeaponStats {
    float range;
    int damage;
    float cooldown;
    FireMode mode;
    float accuracy;
};

// The stats of an item type that can be fired; empty for anything that is not a weapon.
constexpr std::optional<WeaponStats> weapon_stats(ItemType type) {
    if (type == ItemType::Weapon) {
        return WeaponStats{6.0f, 25, 1.0f, FireMode::SemiAutomatic, 0.6f};
    }
    return std::nullopt;
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
        case ItemType::Steel: return "Steel";
        case ItemType::Electronics: return "Electronics";
        case ItemType::Tool: return "Tool";
        case ItemType::Ammo: return "Ammo";
        case ItemType::Weapon: return "Weapon";
        case ItemType::Oil: return "Oil";
        case ItemType::Diesel: return "Diesel";
        case ItemType::Engine: return "Engine";
        case ItemType::IronOre: break;
    }
    return "Iron Ore";
}

} // namespace world
