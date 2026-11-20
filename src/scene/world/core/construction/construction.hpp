#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "scene/world/core/construction/blueprint.hpp"
#include "scene/world/core/construction/build_rule.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

// The families of buildings the player can construct; each one is a tab of the build bar.
enum class ConstructionCategory { Special, Mine, Factory, Military };

// A building the player can construct. `icon` is the name of its texture (see
// graphics::TextureManager); none exists yet, so it is drawn with the fallback texture.
// `type` is empty for an entry that cannot be built yet. `cost` is taken from the player's
// inventory when the construction is placed; `effort` is the work (units, one per second
// today) needed to finish it, 0 = finished at once. What differs between buildings lives
// in two open lists: `rules` (extra conditions to place it, see BuildRule) and `blueprint`
// (the components it is made of: SolidInventory, Producer, Generator, PowerConsumer...).
struct Construction {
    std::string name;
    std::string icon;
    std::optional<BuildingType> type = std::nullopt;
    std::vector<ItemStack> cost = {};
    float effort = 0.0f;
    std::vector<std::shared_ptr<const BuildRule>> rules = {};
    Blueprint blueprint = {};
    // The tiles it covers from the tile it is placed on (see Building).
    TileSize size = {};
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
