#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"

namespace world {

// One end of a logistic route as the player sees it: its building's name and tile.
struct LogisticEnd {
    std::string name;
    TileCoord tile{};
};

// A logistic route with its haulers. `mode` is the mode of its first hauler (none without).
struct LogisticEntry {
    RouteKey key;
    LogisticEnd from;
    LogisticEnd to;
    std::size_t length = 0; // tiles; 0 = unreachable
    int haulers = 0;
    std::optional<HaulMode> mode = std::nullopt;
    BuildingType to_type = BuildingType::Headquarters; // the destination's type
};

// Every Logistic route of the network, grouped by destination type (in BuildingType
// order), in network order within a group.
std::vector<LogisticEntry> logistics_report(const core::Registry& registry,
                                            const RouteNetwork& network);

} // namespace world
