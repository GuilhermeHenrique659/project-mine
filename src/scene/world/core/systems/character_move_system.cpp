#include "scene/world/core/systems/character_move_system.hpp"

#include <cmath>
#include <vector>

#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/events/character_state_changed.hpp"

namespace world {

void CharacterMoveSystem::update(float delta_time) {
    std::vector<core::EntityId> changed;
    registry_.each<Character, Position, Mover>(
        [delta_time, &changed](core::EntityId id, Character& character, Position& position,
                               Mover& mover) {
            const float length = std::hypot(mover.direction.x, mover.direction.y);
            const bool moving = length > 0.0f;
            if (moving) {
                const float speed = mover.running ? mover.speed * RunSpeedMultiplier : mover.speed;
                const float step = speed * delta_time / length;
                position.point.x += mover.direction.x * step;
                position.point.y += mover.direction.y * step;
            }

            CharacterState state = CharacterState::Idle;
            if (moving) {
                state = mover.running ? CharacterState::Running : CharacterState::Moving;
            }
            if (character.state == state) {
                return;
            }
            character.state = state;
            changed.push_back(id);
        });

    // Published after the loop so subscribers are free to change the registry.
    for (const core::EntityId id : changed) {
        bus_.publish(CharacterStateChanged{id});
    }
}

} // namespace world
