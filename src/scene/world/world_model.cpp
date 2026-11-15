#include "scene/world/world_model.hpp"

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/core/systems/build_system.hpp"
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
        registry_.get<::world::Inventory>(player_)->add(::world::items::IronOre,
                                                        ::world::StartingIronOre);
    }
    // Order matters: input writes the direction that movement then reads.
    systems_.push_back(std::make_unique<::world::PlayerInputSystem>(registry_, input));
    systems_.push_back(std::make_unique<::world::CharacterMoveSystem>(registry_, bus_));
    // After movement, so the camera is on the player's new position.
    systems_.push_back(std::make_unique<::world::CameraFollowSystem>(registry_, camera_));
    // Builds on BuildRequested (paid by the player), then advances the construction sites.
    systems_.push_back(
        std::make_unique<::world::BuildSystem>(registry_, bus_, terrain_, factory_, player_));
    systems_.push_back(std::make_unique<::world::ConstructionWorkSystem>(registry_, bus_));
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

void WorldModel::toggle_pause() {
    // The hand that held the mouse button is no longer building.
    work_on(std::nullopt);
    paused_ = !paused_;
}

void WorldModel::toggle_player_menu() {
    if (paused_) {
        return;
    }
    work_on(std::nullopt);
    player_menu_open_ = !player_menu_open_;
}

void WorldModel::toggle_build_bar() {
    if (paused_) {
        return;
    }
    build_bar_open_ = !build_bar_open_;
}

void WorldModel::request_build(::world::BuildingType type, ::world::TileCoord tile) {
    bus_.publish(::world::BuildRequested{type, tile});
}

::world::BuildResult WorldModel::build_status(const ::world::Construction& construction) const {
    return ::world::check_unlocked(registry_, construction);
}

::world::BuildResult WorldModel::placement_status(const ::world::Construction& construction,
                                                  ::world::TileCoord tile) const {
    return ::world::can_build(registry_, construction, tile, terrain_,
                              registry_.get<::world::Inventory>(player_));
}

void WorldModel::work_on(std::optional<::world::TileCoord> tile) {
    ::world::Builder* builder = registry_.get<::world::Builder>(player_);
    if (builder == nullptr) {
        return;
    }
    builder->site = 0;
    if (!tile) {
        return;
    }
    for (const core::Entity& entity : registry_.entities()) {
        const auto* building = registry_.get<::world::Building>(entity.id);
        if (building != nullptr && building->tile == *tile &&
            registry_.has<::world::ConstructionSite>(entity.id)) {
            builder->site = entity.id;
            return;
        }
    }
}

void WorldModel::cancel() {
    if (player_menu_open_) {
        close_player_menu();
        return;
    }
    toggle_pause();
}

} // namespace scene::world
