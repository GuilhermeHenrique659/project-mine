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
// once it reached the logistic route. Saved by the HaulerCodec (see PendingHaulerLinks).
struct Hauler {
    RouteKey route;
    HaulMode mode = HaulMode::Take;
    bool joined = false;
};

// A loaded hauler's route by building tiles (ids are not saved), its mode, whether it had
// joined the route and its walking direction on it. The HaulSystem turns it into a Hauler
// on its next update (a building no longer there = the worker turns idle) and removes it.
struct PendingHaulerLinks {
    TileCoord from{};
    TileCoord to{};
    HaulMode mode = HaulMode::Take;
    bool joined = false;
    bool forward = true;
};

constexpr const char* to_string(HaulMode mode) { return mode == HaulMode::Put ? "Put" : "Take"; }

} // namespace world
