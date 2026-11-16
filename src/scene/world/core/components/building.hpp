#pragma once

#include <optional>
#include <vector>

#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

// How many tiles a building covers along world x (width) and y (height).
struct TileSize {
    int width = 1;
    int height = 1;

    bool operator==(const TileSize&) const = default;
};

// A building standing (or being built) on the `size` tiles from `tile` (its origin, the
// corner with the smallest x and y) towards +x and +y.
struct Building {
    BuildingType type = BuildingType::Headquarters;
    TileCoord tile{};
    TileSize size{};
};

// Present while a building is unfinished: work done so far and the work it needs.
struct ConstructionSite {
    float progress = 0.0f;
    float effort = 0.0f;
};

// What a building produces: every 1 / `rate` seconds it consumes all of `inputs` (empty =
// needs nothing, like a mine) from its inventory and adds one `output` to it. An empty `output`
// means the resource on the ground under the building (the ore under a mine).
struct Recipe {
    std::vector<ItemStack> inputs{};
    std::optional<Item> output = std::nullopt;
    float rate = 0.0f; // outputs per second
};

enum class BuildingState { Working, Idle };

// Why a building is idle; the first that applies, in this order.
enum class IdleReason {
    None,         // working
    NoPower,      // its PowerConsumer is not powered
    NoResource,   // nothing to produce: a mine off ore
    NoWorkers,    // its Staffing needs workers and none is at the building
    MissingInput, // one of the recipe's inputs is not in its inventory
    OutputFull,   // the output does not fit in its inventory
};

// Whether a finished building works and, when idle, why. Its systems set it every frame
// (the ProductionSystem for a producer); not saved.
struct BuildingStatus {
    BuildingState state = BuildingState::Working;
    IdleReason reason = IdleReason::None;
};

// A building that produces (see Recipe). It holds its own copy of the recipe so the rate
// can change per building; `progress` counts towards the next output (1 = one output).
struct Producer {
    Recipe recipe;
    float progress = 0.0f;
};

} // namespace world
