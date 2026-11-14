#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"

namespace world {

// Moves every character (an entity with Character, Position and Mover) along its
// direction at its speed and keeps its state: Moving while it has a direction, Idle
// otherwise. Publishes CharacterStateChanged only when the state changes. Entities
// without the Character component are left alone.
// The direction is normalized so diagonals are not faster.
void character_move_system(core::Registry& registry, event::EventBus& bus, float delta_time);

} // namespace world
