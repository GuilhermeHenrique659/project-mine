#pragma once

#include "scene/world/core/isometric.hpp"

namespace world {

// Marks a character the simulation drives (never the player): the base archetype of every
// unit, see spawn_unit(). A worker is a unit with a Worker on top; future units (soldiers,
// ...) are units with their own component. Systems that concern every unit select on it.
struct Unit {};

// How a unit is getting on in traffic, kept by the UnitTrafficSystem (added on its first
// update; not saved). `stuck` counts the seconds it has tried to walk without getting
// anywhere; past StuckTime it becomes a `ghost`: it walks through units and buildings until
// it is a tile away from `ghost_from`, so no jam lasts. `last` is where it stood on the
// previous update. `spread` is how far (tiles per second) a standing unit is pushed away
// from the units it overlaps; the CharacterMoveSystem applies it only into free space.
struct UnitTraffic {
    WorldPoint last{};
    bool tracked = false;
    float stuck = 0.0f;
    bool ghost = false;
    WorldPoint ghost_from{};
    WorldPoint spread{};
};

} // namespace world
