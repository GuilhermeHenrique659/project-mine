#pragma once

namespace world {

// Marks a unit as a soldier (see spawn_soldier). Only the marker is saved, through the
// soldier archetype; its inventory is saved like any other.
struct Soldier {};

// A building where the player recruits soldiers (SoldierRecruitRequested, see
// RecruitSystem): only once finished and powered.
struct Recruiter {};

} // namespace world
