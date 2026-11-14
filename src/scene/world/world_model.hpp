#pragma once

#include <utility>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "game/save_game.hpp"
#include "scene/scene.hpp"
#include "world/isometric.hpp"

namespace scene::world {

// State of the game being played: the save it came from and the simulation (an ECS
// registry with the player character). The simulation lives in core:: and ::world::
// and knows nothing about input or rendering; this model only drives it.
class WorldModel : public scene::Model {
public:
    // The bus receives the simulation events (::world::CharacterMoved) and must outlive
    // the model.
    WorldModel(event::EventBus& bus, game::SaveGame save);

    // Runs the systems for one frame.
    void update(float delta_time) override;

    // Sets where the player wants to walk, in world space ((0, 0) = stand still). The
    // only entry point for input.
    void set_player_direction(::world::WorldPoint direction);

    // Everything that has to be written to disk to continue this game later.
    const game::SaveGame& save_game() const { return save_; }

    // Read-only access to the simulation for the view.
    const core::Registry& registry() const { return registry_; }
    core::EntityId player_id() const { return player_; }

private:
    event::EventBus& bus_;
    game::SaveGame save_;
    core::Registry registry_;
    core::EntityId player_ = 0;
};

} // namespace scene::world
