#include "scene/world/world_model.hpp"

#include "world/components/mover.hpp"
#include "world/entities/character.hpp"
#include "world/systems/character_move_system.hpp"

namespace scene::world {

WorldModel::WorldModel(event::EventBus& bus, game::SaveGame save)
    : bus_(bus),
      save_(std::move(save)),
      player_(::world::spawn_character(registry_, "Player", ::world::WorldPoint{0.0f, 0.0f})) {}

void WorldModel::update(float delta_time) {
    if (paused_) {
        return;
    }
    ::world::character_move_system(registry_, bus_, delta_time);
}

void WorldModel::toggle_pause() { paused_ = !paused_; }

void WorldModel::set_player_direction(::world::WorldPoint direction) {
    ::world::Mover* mover = registry_.get<::world::Mover>(player_);
    if (mover == nullptr) {
        return;
    }
    mover->direction = direction;
}

} // namespace scene::world
