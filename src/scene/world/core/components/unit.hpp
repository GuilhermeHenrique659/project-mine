#pragma once

namespace world {

// Marks a character the simulation drives (never the player): the base archetype of every
// unit, see spawn_unit(). A worker is a unit with a Worker on top; future units (soldiers,
// ...) are units with their own component. Systems that concern every unit select on it.
struct Unit {};

} // namespace world
