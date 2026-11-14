#pragma once

#include <memory>

#include "scene/scene.hpp"

namespace game {
class Game;
}

namespace scene::settings {

// Settings screen, opened from the main menu. Starts from game.display_settings().
std::unique_ptr<scene::Scene> make_settings_scene(game::Game& game);

} // namespace scene::settings
