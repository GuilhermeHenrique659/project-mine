#pragma once

#include <memory>

#include "scene/scene.hpp"

namespace game {
class Game;
}

namespace scene::menu {

// First scene shown on start up.
std::unique_ptr<scene::Scene> make_title_scene(game::Game& game);

} // namespace scene::menu
