#include "scene/world/world_model.hpp"

#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/systems/player_input_system.hpp"

namespace scene::world {

WorldModel::WorldModel(event::EventBus& bus, const input::InputHandler& input,
                       game::SaveGame save)
    : bus_(bus),
      save_(std::move(save)),
      player_(::world::spawn_character(registry_, "Player", ::world::WorldPoint{0.0f, 0.0f})) {
    registry_.add(player_, ::world::PlayerControlled{});
    // Order matters: input writes the direction that movement then reads.
    systems_.push_back(std::make_unique<::world::PlayerInputSystem>(registry_, input));
    systems_.push_back(std::make_unique<::world::CharacterMoveSystem>(registry_, bus_));
}

void WorldModel::update(float delta_time) {
    if (paused_) {
        return;
    }
    for (const auto& system : systems_) {
        system->update(delta_time);
    }
}

void WorldModel::toggle_pause() { paused_ = !paused_; }

} // namespace scene::world
