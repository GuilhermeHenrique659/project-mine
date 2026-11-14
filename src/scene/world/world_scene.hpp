#pragma once

#include <memory>

#include "game/save_game.hpp"
#include "scene/scene.hpp"

namespace game {
class Game;
}

namespace scene::world {

// Builds the in-game scene for a new or loaded save.
std::unique_ptr<scene::Scene> make_world_scene(game::Game& game, game::SaveGame save);

} // namespace scene::world
