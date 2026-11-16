#pragma once

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "game/save_game.hpp"
#include "input/input_handler.hpp"
#include "scene/scene.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/core/construction/build_rules.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/save/world_serializer.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace scene::world {

// State of the game being played: the save it came from and the simulation (an ECS
// registry with the player character). The simulation lives in core:: and ::world::
// and knows nothing about rendering; this model only drives it. Input reaches it through
// the handler injected in the constructor, read by the input system.
class WorldModel : public scene::Model {
public:
    // The bus receives the simulation events (::world::CharacterStateChanged) and the
    // input is read by the systems each update; both must outlive the model.
    WorldModel(event::EventBus& bus, const input::InputHandler& input, game::SaveGame save);

    // The serializer and the systems refer to sibling members, so the model never moves.
    WorldModel(const WorldModel&) = delete;
    WorldModel& operator=(const WorldModel&) = delete;

    // Runs the systems for one frame.
    void update(float delta_time) override;

    // Opens or closes the pause. While paused update() does not run the simulation.
    void toggle_pause();
    bool is_paused() const { return paused_; }

    // Opens or closes the player menu (inventory). Ignored while paused; the simulation
    // keeps running while it is open.
    void toggle_player_menu();
    bool is_player_menu_open() const { return player_menu_open_; }
    void close_player_menu() { player_menu_open_ = false; }

    // Opens or closes the build bar of the HUD (B). Ignored while paused.
    void toggle_build_bar();
    bool is_build_bar_open() const { return build_bar_open_; }

    // Asks to place a building on a tile (::world::BuildRequested on the bus); the build
    // system builds it when the rules allow, paid by the player.
    void request_build(::world::BuildingType type, ::world::TileCoord tile);

    // Whether a construction can be built at all now (headquarters rule, uniqueness),
    // regardless of tile and cost: the build bar disables the others.
    ::world::BuildResult build_status(const ::world::Construction& construction) const;

    // Every rule for placing `construction` on `tile` now, paid by the player (the build
    // preview shows it).
    ::world::BuildResult placement_status(const ::world::Construction& construction,
                                          ::world::TileCoord tile) const;

    // Points the player at the construction site on `tile` (none = stops working; a tile
    // without a site also stops). The work itself happens in update(), only while the
    // player is within ::world::BuildReach of the site. Pausing or opening the player
    // menu stops the work.
    void work_on(std::optional<::world::TileCoord> tile);

    // Opens the item transfer with the finished building on `tile` when it has an
    // inventory and is within ::world::TransferReach of the player; otherwise nothing.
    void open_transfer(::world::TileCoord tile);
    // The building the transfer is open with, 0 when closed. It closes by itself when the
    // player walks out of reach or the building is gone.
    core::EntityId transfer_target() const { return transfer_target_; }
    void close_transfer() { transfer_target_ = 0; }

    // Asks to move items between two inventories (::world::ItemTransferRequested).
    void request_transfer(core::EntityId from, core::EntityId to, ::world::Item item, int count);

    // Asks to send `count` idle workers to a production building, or with a negative count
    // to send that many of its workers back (::world::WorkerAssignmentRequested).
    void request_workers(core::EntityId building, int count);

    // The "back" action (Esc): closes the transfer or the player menu when one is open,
    // otherwise opens or closes the pause.
    void cancel();

    // Everything that has to be written to disk to continue this game later: the save's
    // header plus a snapshot of the persistent entities as they are now.
    game::SaveGame save_game() const;

    // Name, creation time and seed only, without snapshotting the entities.
    const game::SaveGame& save_header() const { return save_; }

    // Read-only access to the simulation for the view.
    const core::Registry& registry() const { return registry_; }
    core::EntityId player_id() const { return player_; }

    // The ground of this world, generated from the save's seed.
    const ::world::Terrain& terrain() const { return terrain_; }

    // Where the view is centred: it follows the player.
    const ::world::Camera& camera() const { return camera_; }

    // The precomputed routes between buildings (entities follow them with a RouteFollower).
    const ::world::RouteNetwork& routes() const { return routes_; }

    // Zooms the camera by mouse wheel notches (positive = in). Ignored while paused.
    void zoom_camera(float steps);

private:
    // The first PlayerControlled entity, or 0 when there is none (ids start at 1).
    core::EntityId find_player() const;

    event::EventBus& bus_;
    // Header data only (name, creation time, seed); the entities live in registry_.
    game::SaveGame save_;
    ::world::Terrain terrain_;
    // Declared before registry_ and used by the constructor to fill it.
    ::world::EntityFactory factory_;
    ::world::WorldSerializer serializer_;
    core::Registry registry_;
    core::EntityId player_ = 0;
    ::world::Camera camera_;
    // Routes between buildings (headquarters -> each building); rebuilt by the RouteSystem.
    ::world::RouteNetwork routes_{std::make_unique<::world::HeadquartersRoutePlanner>()};
    // Declared after registry_ and bus_, which the systems refer to, so they are destroyed first.
    std::vector<std::unique_ptr<::world::System>> systems_;
    bool paused_ = false;
    bool player_menu_open_ = false;
    bool build_bar_open_ = false;
    core::EntityId transfer_target_ = 0;
};

} // namespace scene::world
