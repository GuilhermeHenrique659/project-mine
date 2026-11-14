#include "world/systems/player_input_system.hpp"

#include "input/action.hpp"
#include "world/components/mover.hpp"
#include "world/components/player_controlled.hpp"
#include "world/isometric.hpp"

namespace world {

namespace {

// Movement the player asks for, in screen space (y grows downwards).
ScreenPoint held_direction(const input::InputHandler& input) {
    ScreenPoint direction;
    if (input.is_held(input::Action::MoveUp)) direction.y -= 1.0f;
    if (input.is_held(input::Action::MoveDown)) direction.y += 1.0f;
    if (input.is_held(input::Action::MoveLeft)) direction.x -= 1.0f;
    if (input.is_held(input::Action::MoveRight)) direction.x += 1.0f;
    return direction;
}

} // namespace

void PlayerInputSystem::update(float /*delta_time*/) {
    const WorldPoint direction = screen_direction_to_world(held_direction(input_));
    registry_.each<PlayerControlled, Mover>(
        [direction](core::EntityId, PlayerControlled&, Mover& mover) {
            mover.direction = direction;
        });
}

} // namespace world
