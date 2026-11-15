#pragma once

#include <string>
#include <vector>

namespace world {

// The families of buildings the player can construct; each one is a tab of the build bar.
enum class ConstructionCategory { Special, Mine, Factory, Military };

// A building the player can construct. `icon` is the name of its texture (see
// graphics::TextureManager); none exists yet, so it is drawn with the fallback texture.
struct Construction {
    std::string name;
    std::string icon;
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

} // namespace world
