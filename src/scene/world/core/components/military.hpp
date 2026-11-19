#pragma once

#include "core/entity.hpp"

namespace world {

// Marks a unit as a soldier (see spawn_soldier). Only the marker is saved, through the
// soldier archetype; its inventory is saved like any other.
struct Soldier {};

// A building where the player recruits soldiers (SoldierRecruitRequested, see
// RecruitSystem): only once finished and powered.
struct Recruiter {};

// A building soldiers resupply at (the Barracks): its inventory has a section filtered to
// Weapon and one to Ammo, and an ordered soldier takes a weapon and an ammo stack from them
// (see ResupplySystem).
struct Supply {};

// A soldier walking to `supply` to take a weapon and an ammo stack there (see
// ResupplySystem). Not saved.
struct ResupplyOrder {
    core::EntityId supply = 0;
};

} // namespace world
