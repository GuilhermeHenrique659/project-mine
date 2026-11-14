#pragma once

#include "core/entity.hpp"

namespace world {

// Published by the character move system when a character's state changes (it starts
// or stops moving). Read the new state from the entity's Character component.
struct CharacterStateChanged {
    core::EntityId entity = 0;
};

} // namespace world
