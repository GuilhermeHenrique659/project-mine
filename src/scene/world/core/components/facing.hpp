#pragma once

#include "scene/world/core/isometric.hpp"

namespace world {

// The eight compass directions a character can face, as seen on screen: North is screen
// up, East screen right. In order, clockwise from North (sprite sheet rows use it).
enum class Direction { North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest };

inline constexpr int DirectionCount = 8;

// Where a character looks. Every character has one (spawn_character()); the
// CharacterMoveSystem turns it towards where it walks, and it stays when it stops. Not saved.
struct Facing {
    Direction direction = Direction::South;
};

// The screen direction closest to a movement along world `direction` (not normalized).
// A zero direction gives South.
Direction facing_of(WorldPoint direction);

} // namespace world
