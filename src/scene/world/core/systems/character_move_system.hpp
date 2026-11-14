#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Moves every character (an entity with Character, Position and Mover) along its
// direction at its speed and keeps its state: Moving while it has a direction, Idle
// otherwise. Publishes CharacterStateChanged only when the state changes. Entities
// without the Character component are left alone.
// The direction is normalized so diagonals are not faster.
// The registry and the bus must outlive the system.
class CharacterMoveSystem : public System {
public:
    CharacterMoveSystem(core::Registry& registry, event::EventBus& bus)
        : registry_(registry), bus_(bus) {}

    void update(float delta_time) override;

private:
    core::Registry& registry_;
    event::EventBus& bus_;
};

} // namespace world
