#pragma once

#include <vector>

#include "core/entity.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// The player asks to recruit one soldier at a building (a barracks). The RecruitSystem
// spawns it on the building when it is a finished, powered Recruiter; otherwise nothing.
struct SoldierRecruitRequested {
    core::EntityId building = 0;
};

// The player orders units (the selection) to walk to a tile. The MoveOrderSystem gives each
// soldier among them a path to its own tile around `goal` (see formation_tiles).
struct UnitsMoveRequested {
    std::vector<core::EntityId> units;
    TileCoord goal;
};

// The player orders units (the selection) to resupply at a Supply building. The
// ResupplySystem walks each soldier among them next to it, where it takes a weapon and an
// ammo stack.
struct UnitsResupplyRequested {
    std::vector<core::EntityId> units;
    core::EntityId supply = 0;
};

} // namespace world
