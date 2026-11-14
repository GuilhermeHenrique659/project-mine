#pragma once

#include <string>

namespace world {

// Marks an entity that goes into the save. `archetype` is how the EntityFactory respawns
// it on load. Entities without it (transient ones) are not saved.
struct Persistent {
    std::string archetype;
};

} // namespace world
