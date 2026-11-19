#pragma once

#include "core/entity.hpp"

namespace world {

// The player asks to recruit one soldier at a building (a barracks). The RecruitSystem
// spawns it on the building when it is a finished, powered Recruiter; otherwise nothing.
struct SoldierRecruitRequested {
    core::EntityId building = 0;
};

} // namespace world
