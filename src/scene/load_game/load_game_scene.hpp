#pragma once

#include <memory>

#include "scene/scene.hpp"

namespace game {
class Game;
}

namespace scene::load_game {

// Builds the Load Game screen listing the saves currently on disk.
std::unique_ptr<scene::Scene> make_load_game_scene(game::Game& game);

} // namespace scene::load_game
