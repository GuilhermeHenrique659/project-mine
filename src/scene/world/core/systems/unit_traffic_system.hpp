#pragma once

#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Moving units closer than this (centre to centre, in tiles) push each other aside.
inline constexpr float SeparationRadius = 0.375f;

// The largest sideways part a push adds to a unit's (unit length) walking direction, so a
// unit always keeps most of its speed forward and is never pinned in place.
inline constexpr float MaxSidewaysPush = 0.5f;

// Seconds a unit may try to walk without getting anywhere before it walks through what
// blocks it (see UnitTraffic).
inline constexpr float StuckTime = 2.0f;

// Tiles per second at which overlapping standing units drift apart (at most).
inline constexpr float SpreadSpeed = 1.0f;

struct Walker {
    core::EntityId id;
    WorldPoint at;
    WorldPoint heading; // unit length
};

// Keeps walking units (Unit + Position + Mover with a direction) from piling up, without
// ever blocking one: each moving unit is pushed away from the moving units within
// SeparationRadius, sideways only (perpendicular to where it walks; straight ahead or
// behind pushes it to its right), capped at MaxSidewaysPush. Standing units (workers waiting
// or at work) neither move nor push. Neighbours are found through a per-tile index.
// It also keeps each unit's UnitTraffic: a unit trying to walk for StuckTime without
// getting anywhere becomes a ghost (no pushes, no collision in the CharacterMoveSystem)
// until it is a tile further, so no jam can last.
// Standing units closer than SeparationRadius to another unit get a UnitTraffic::spread
// away from it (up to SpreadSpeed); the CharacterMoveSystem applies it only into free
// space, so a crowd fans out where there is room and stays packed where there is none.
// Runs after RouteFollowSystem (which sets the directions) and before CharacterMoveSystem.
// The registry must outlive it.
class UnitTrafficSystem : public System {
public:
    explicit UnitTrafficSystem(core::Registry& registry) : registry_(registry) {}

    void update(float delta_time) override;

private:
    // Counts how long a walking unit has been stuck and turns it into a ghost, or back.
    static void track(UnitTraffic& traffic, WorldPoint at, const Mover& mover, bool moving,
                      float delta_time);

    // Sets the spread of every standing unit among `bodies`.
    void spread(const std::vector<Walker>& bodies);

    core::Registry& registry_;
};

} // namespace world
