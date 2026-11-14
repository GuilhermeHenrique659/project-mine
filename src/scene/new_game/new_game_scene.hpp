#pragma once

#include <memory>

#include "scene/scene.hpp"

namespace game {
class Game;
}

namespace scene::new_game {

// Builds the New Game screen (name prompt).
std::unique_ptr<scene::Scene> make_new_game_scene(game::Game& game);

} // namespace scene::new_game
