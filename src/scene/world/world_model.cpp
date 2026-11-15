#include "scene/world/world_model.hpp"

#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/systems/camera_follow_system.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/systems/player_input_system.hpp"

namespace scene::world {

WorldModel::WorldModel(event::EventBus& bus, const input::InputHandler& input,
                       game::SaveGame save)
    : bus_(bus),
      save_(std::move(save)),
      terrain_(save_.seed),
      factory_(::world::make_entity_factory()),
      serializer_(::world::make_world_serializer(factory_)) {
    serializer_.restore(registry_, save_.entities);
    save_.entities.clear();
    player_ = find_player();
    if (player_ == 0) {
        player_ = *factory_.spawn(registry_, "character", "Player");
        registry_.add(player_, ::world::PlayerControlled{});
    }
    // Order matters: input writes the direction that movement then reads.
    systems_.push_back(std::make_unique<::world::PlayerInputSystem>(registry_, input));
    systems_.push_back(std::make_unique<::world::CharacterMoveSystem>(registry_, bus_));
    // After movement, so the camera is on the player's new position.
    systems_.push_back(std::make_unique<::world::CameraFollowSystem>(registry_, camera_));
    // The first frame is already centred on the player.
    if (const auto* position = registry_.get<::world::Position>(player_)) {
        camera_.center = position->point;
    }
}

core::EntityId WorldModel::find_player() const {
    for (const core::Entity& entity : registry_.entities()) {
        if (registry_.has<::world::PlayerControlled>(entity.id)) {
            return entity.id;
        }
    }
    return 0;
}

game::SaveGame WorldModel::save_game() const {
    game::SaveGame save = save_;
    save.entities = serializer_.snapshot(registry_);
    return save;
}

void WorldModel::update(float delta_time) {
    if (paused_) {
        return;
    }
    for (const auto& system : systems_) {
        system->update(delta_time);
    }
}

void WorldModel::zoom_camera(float steps) {
    if (paused_) {
        return;
    }
    camera_.zoom_by(steps);
}

void WorldModel::toggle_pause() { paused_ = !paused_; }

void WorldModel::toggle_player_menu() {
    if (paused_) {
        return;
    }
    player_menu_open_ = !player_menu_open_;
}

void WorldModel::toggle_build_bar() {
    if (paused_) {
        return;
    }
    build_bar_open_ = !build_bar_open_;
}

void WorldModel::cancel() {
    if (player_menu_open_) {
        close_player_menu();
        return;
    }
    toggle_pause();
}

} // namespace scene::world
