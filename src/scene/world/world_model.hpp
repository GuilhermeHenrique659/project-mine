#pragma once

#include <utility>

#include "game/save_game.hpp"
#include "scene/scene.hpp"

namespace scene::world {

// State of the game being played. For now only the save it came from; the simulation
// (map, entities, production) grows here.
class WorldModel : public scene::Model {
public:
    explicit WorldModel(game::SaveGame save) : save_(std::move(save)) {}

    void update(float) override {}

    // Everything that has to be written to disk to continue this game later.
    const game::SaveGame& save_game() const { return save_; }

private:
    game::SaveGame save_;
};

} // namespace scene::world
