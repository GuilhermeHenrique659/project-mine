#include "scene/world/world_model.hpp"

#include <algorithm>

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/core/events/item_transfer_requested.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/events/worker_events.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/systems/build_system.hpp"
#include "scene/world/core/systems/inventory_transfer_system.hpp"
#include "scene/world/core/systems/power_system.hpp"
#include "scene/world/core/systems/production_system.hpp"
#include "scene/world/core/systems/recruit_system.hpp"
#include "scene/world/core/systems/shoot_system.hpp"
#include "scene/world/core/events/combat_events.hpp"
#include "scene/world/core/targeting.hpp"
#include "scene/world/core/systems/camera_follow_system.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/systems/haul_system.hpp"
#include "scene/world/core/systems/player_input_system.hpp"
#include "scene/world/core/systems/route_follow_system.hpp"
#include "scene/world/core/components/selected.hpp"
#include "scene/world/core/selection.hpp"
#include "scene/world/core/systems/move_order_system.hpp"
#include "scene/world/core/systems/resupply_system.hpp"
#include "scene/world/core/systems/path_follow_system.hpp"
#include "scene/world/core/systems/unit_traffic_system.hpp"
#include "scene/world/core/systems/route_system.hpp"
#include "scene/world/core/systems/worker_system.hpp"

namespace scene::world {

WorldModel::WorldModel(event::EventBus& bus, const input::InputHandler& input,
                       game::SaveGame save, ::world::HitRoll roll)
    : bus_(bus),
      save_(std::move(save)),
      terrain_(save_.seed),
      factory_(::world::make_entity_factory()),
      serializer_(::world::make_world_serializer(factory_)) {
    ::world::track_buildings(registry_);
    serializer_.restore(registry_, save_.entities);
    save_.entities.clear();
    player_ = find_player();
    if (player_ == 0) {
        player_ = *factory_.spawn(registry_, "character", "Player");
        registry_.add(player_, ::world::PlayerControlled{});
        registry_.get<::world::Inventory>(player_)->add(::world::items::IronOre,
                                                        ::world::StartingIronOre);
        registry_.get<::world::Inventory>(player_)->add(::world::items::Weapon,
                                                        ::world::StartingWeapons);
        registry_.get<::world::Inventory>(player_)->add(::world::items::Ammo,
                                                        ::world::StartingAmmo);
    }
    shot_subscription_ = event::subscribe_scoped<::world::ShotFired>(
        bus_, [this](const ::world::ShotFired& shot) {
            const auto* from = registry_.get<::world::Position>(shot.shooter);
            const auto* to = registry_.get<::world::Position>(shot.target);
            if (from == nullptr || to == nullptr) {
                return;
            }
            shot_traces_.push_back({from->point, to->point, 0.0f});
        });
    // Only the player and soldiers shoot (a saved player is armed here too).
    registry_.add(player_, ::world::Shooter{});
    // Order matters: input writes the direction that movement then reads.
    systems_.push_back(std::make_unique<::world::PlayerInputSystem>(registry_, input));
    // Routes are recomputed only on the first frame and after a building is placed; the
    // followers then steer along them, before movement reads their direction.
    systems_.push_back(
        std::make_unique<::world::RouteSystem>(registry_, bus_, terrain_, routes_));
    // Workers spawn with the headquarters and take free construction sites, then follow
    // the route to them.
    systems_.push_back(std::make_unique<::world::WorkerSystem>(registry_, bus_, factory_));
    systems_.push_back(std::make_unique<::world::RecruitSystem>(registry_, bus_, factory_, terrain_));
    // Move orders give the soldiers a path of their own, walked like a route.
    systems_.push_back(std::make_unique<::world::ShootSystem>(registry_, bus_, std::move(roll)));
    systems_.push_back(std::make_unique<::world::MoveOrderSystem>(registry_, bus_, terrain_));
    systems_.push_back(std::make_unique<::world::ResupplySystem>(registry_, bus_, terrain_));
    systems_.push_back(std::make_unique<::world::RouteFollowSystem>(registry_, routes_));
    systems_.push_back(std::make_unique<::world::PathFollowSystem>(registry_));
    systems_.push_back(std::make_unique<::world::UnitTrafficSystem>(registry_));
    systems_.push_back(std::make_unique<::world::CharacterMoveSystem>(registry_, bus_, &terrain_));
    systems_.push_back(std::make_unique<::world::HaulSystem>(registry_, bus_, routes_));
    // After movement, so the camera is on the player's new position.
    systems_.push_back(std::make_unique<::world::CameraFollowSystem>(registry_, camera_));
    // Builds on BuildRequested (paid by the player), then advances the construction sites.
    systems_.push_back(
        std::make_unique<::world::BuildSystem>(registry_, bus_, terrain_, factory_, player_));
    systems_.push_back(std::make_unique<::world::ConstructionWorkSystem>(registry_, bus_));
    systems_.push_back(std::make_unique<::world::InventoryTransferSystem>(registry_, bus_));
    systems_.push_back(std::make_unique<::world::PowerSystem>(registry_));
    systems_.push_back(std::make_unique<::world::ProductionSystem>(registry_, terrain_));
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
    // Paused time is not banked: resuming does not run the ticks it would have made.
    if (paused_) {
        return;
    }
    // Age the traces first, so a shot fired in this update is still drawn.
    for (ShotTrace& trace : shot_traces_) {
        trace.age += delta_time;
    }
    std::erase_if(shot_traces_,
                  [](const ShotTrace& trace) { return trace.age >= ShotTraceDuration; });
    const int ticks = clock_.advance(delta_time);
    for (int tick = 0; tick < ticks; ++tick) {
        run_tick();
    }
    if (transfer_target_ != 0 && !::world::within_transfer_reach(registry_, player_, transfer_target_)) {
        close_transfer();
    }
}

void WorldModel::run_tick() {
    // Requests published by a handler (a request made from a subscriber) wait a tick.
    auto requests = std::move(pending_);
    pending_.clear();
    for (const auto& publish : requests) {
        publish(bus_);
    }
    for (const auto& system : systems_) {
        system->update(SimulationTick);
    }
}

void WorldModel::zoom_camera(float steps) {
    // While a transfer or the player menu is open the wheel scrolls its panel instead.
    if (paused_ || transfer_target() != 0 || player_menu_open_) {
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
    enqueue(::world::BuildRequested{type, tile});
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
    const core::EntityId building = ::world::building_at(registry_, *tile);
    if (registry_.has<::world::ConstructionSite>(building)) {
        builder->site = building;
    }
}

void WorldModel::open_transfer(::world::TileCoord tile) {
    const core::EntityId building = ::world::building_at(registry_, tile);
    if (building == 0 || registry_.has<::world::ConstructionSite>(building) ||
        !registry_.has<::world::Inventory>(building) ||
        !::world::within_transfer_reach(registry_, player_, building)) {
        return;
    }
    transfer_target_ = building;
}

void WorldModel::request_transfer(core::EntityId from, core::EntityId to, ::world::Item item,
                                  int count) {
    enqueue(::world::ItemTransferRequested{from, to, item, count});
}

void WorldModel::request_workers(core::EntityId building, int count) {
    enqueue(::world::WorkerAssignmentRequested{building, count});
}

void WorldModel::request_recruit(core::EntityId building) {
    enqueue(::world::SoldierRecruitRequested{building});
}

void WorldModel::request_haulers(::world::RouteKey route, ::world::HaulMode mode, int count) {
    enqueue(::world::HaulersRequested{route, mode, count});
}

void WorldModel::request_haul_mode(::world::RouteKey route, ::world::HaulMode mode) {
    enqueue(::world::HaulModeRequested{route, mode});
}

void WorldModel::select_units(const ::world::IsoProjection& projection, ::world::ScreenPoint a,
                              ::world::ScreenPoint b) {
    clear_selection();
    for (const core::EntityId id : ::world::soldiers_in(registry_, projection, a, b)) {
        registry_.add(id, ::world::Selected{});
    }
}

void WorldModel::clear_selection() {
    for (const core::EntityId id : selected_units()) {
        registry_.remove<::world::Selected>(id);
    }
}

std::vector<core::EntityId> WorldModel::selected_units() const {
    std::vector<core::EntityId> selected;
    for (const core::Entity& entity : registry_.entities()) {
        if (registry_.has<::world::Selected>(entity.id)) {
            selected.push_back(entity.id);
        }
    }
    return selected;
}

void WorldModel::order_move(::world::TileCoord goal) {
    std::vector<core::EntityId> units = selected_units();
    if (units.empty()) {
        return;
    }
    enqueue(::world::UnitsMoveRequested{std::move(units), goal});
}

void WorldModel::shoot_at(::world::TileCoord tile) {
    const core::EntityId target = ::world::shootable_at(registry_, tile, player_);
    if (target == 0) {
        return;
    }
    enqueue(::world::ShotRequested{player_, target});
}

void WorldModel::hold_trigger(::world::TileCoord tile) {
    const auto* inventory = registry_.get<::world::Inventory>(player_);
    if (inventory == nullptr) {
        return;
    }
    const auto weapon = ::world::held_weapon(*inventory);
    if (!weapon || weapon->mode != ::world::FireMode::Automatic) {
        return;
    }
    shoot_at(tile);
}

void WorldModel::order_units(::world::TileCoord tile) {
    const std::vector<core::EntityId> units = selected_units();
    const core::EntityId supply = ::world::building_at(registry_, tile);
    if (supply != 0 && registry_.has<::world::Supply>(supply)) {
        enqueue(::world::UnitsResupplyRequested{units, supply});
        return;
    }
    const core::EntityId target = ::world::shootable_at(registry_, tile, player_);
    if (target == 0 || std::find(units.begin(), units.end(), target) != units.end()) {
        order_move(tile);
        return;
    }
    enqueue(::world::UnitsAttackRequested{units, target});
}

void WorldModel::cancel() {
    if (transfer_target_ != 0) {
        close_transfer();
        return;
    }
    if (player_menu_open_) {
        close_player_menu();
        return;
    }
    toggle_pause();
}

} // namespace scene::world
