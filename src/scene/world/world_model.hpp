#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "game/save_game.hpp"
#include "input/input_handler.hpp"
#include "scene/scene.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/isometric.hpp"
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
    // Declared after registry_ and bus_, which the systems refer to, so they are destroyed first.
    std::vector<std::unique_ptr<::world::System>> systems_;
    bool paused_ = false;
};

} // namespace scene::world
