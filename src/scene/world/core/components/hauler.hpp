#pragma once

#include "scene/world/core/pathfinding/route_network.hpp"

namespace world {

// Which way a logistic route carries items: Take = from its production building into its
// storage; Put = from its storage into its production building.
enum class HaulMode { Take, Put };

// A worker assigned (by the player, HaulersRequested) to a logistic route (from = the
// storage, to = the production building). It walks the headquarters' route to the storage,
// then loops along the logistic route forever: at the source end it fills its one-slot
// inventory, at the other end it unloads what fits (see HaulSystem). `joined` turns true
// once it reached the logistic route. Not saved: a loaded worker is idle.
struct Hauler {
    RouteKey route;
    HaulMode mode = HaulMode::Take;
    bool joined = false;
};

constexpr const char* to_string(HaulMode mode) { return mode == HaulMode::Put ? "Put" : "Take"; }

} // namespace world
