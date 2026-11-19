#include "scene/new_game/new_game_scene.hpp"

#include "game/game.hpp"
#include "input/raylib_keyboard.hpp"
#include "input/raylib_pointer.hpp"
#include "scene/new_game/new_game_controller.hpp"
#include "scene/new_game/new_game_view.hpp"

namespace scene::new_game {

std::unique_ptr<scene::Scene> make_new_game_scene(game::Game& game) {
    // The typed name lives in the view's field, so the scene has no model.
    auto view = std::make_unique<NewGameView>(game.event_bus());
    auto controller = std::make_unique<NewGameController>(
        *view, game.event_bus(), std::make_unique<input::RaylibKeyboard>(),
        std::make_shared<input::RaylibPointer>());
    return std::make_unique<scene::Scene>(nullptr, std::move(view), std::move(controller));
}

} // namespace scene::new_game
