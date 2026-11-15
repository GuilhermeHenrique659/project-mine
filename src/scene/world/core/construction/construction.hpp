#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "scene/world/core/items/item.hpp"

namespace world {

// The families of buildings the player can construct; each one is a tab of the build bar.
enum class ConstructionCategory { Special, Mine, Factory, Military };

// The buildings that exist in the simulation. One value per implemented building; the
// catalog lists others without a type until they are implemented.
enum class BuildingType { Headquarters, Mine };

// Stable save-file name of a building type ("Headquarters").
std::string_view building_type_key(BuildingType type);
std::optional<BuildingType> parse_building_type(std::string_view key);

// A building the player can construct. `icon` is the name of its texture (see
// graphics::TextureManager); none exists yet, so it is drawn with the fallback texture.
// `type` is empty for an entry that cannot be built yet. `cost` is taken from the player's
// inventory when the construction is placed; `effort` is the work (units, one per second
// today) needed to finish it, 0 = finished at once. A `unique` building exists at most once.
// Only a construction with `needs_headquarters` false (the headquarters itself) can be
// built before a headquarters exists. A construction that `needs_ore` stands only on an
// ore tile (iron, copper or coal).
struct Construction {
    std::string name;
    std::string icon;
    std::optional<BuildingType> type = std::nullopt;
    std::vector<ItemStack> cost = {};
    float effort = 0.0f;
    bool unique = false;
    bool needs_headquarters = true;
    bool needs_ore = false;
    // Slots of the building's own inventory (0 = it has none).
    std::size_t inventory_slots = 0;
};

// The constructions of one category, with the category's own name and icon.
struct ConstructionGroup {
    ConstructionCategory category;
    std::string name;
    std::string icon;
    std::vector<Construction> constructions;
};

// Every construction of the game, grouped by category, in build bar order:
// Special, Mines, Factories, Military.
const std::vector<ConstructionGroup>& construction_catalog();

// The catalog entry of a building type (always found for a valid type).
const Construction* find_construction(BuildingType type);

} // namespace world
