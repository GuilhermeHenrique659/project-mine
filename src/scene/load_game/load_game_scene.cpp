#include "scene/load_game/load_game_scene.hpp"

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/raylib_keyboard.hpp"
#include "input/raylib_pointer.hpp"
#include "scene/load_game/load_game_view.hpp"

namespace scene::load_game {

std::unique_ptr<scene::Scene> make_load_game_scene(game::Game& game) {
    // The list is read once, when the screen opens; the scene keeps no state of its own.
    auto view = std::make_unique<LoadGameView>(game.event_bus(), game.save_store().list());
    auto controller = std::make_unique<scene::PanelController>(
        *view, std::make_unique<input::RaylibKeyboard>(), input::InputMap::menu_defaults(),
        std::make_shared<input::RaylibPointer>());
    controller->on_cancel([&game] { game.event_bus().publish(game::BackToMenuRequested{}); });
    return std::make_unique<scene::Scene>(nullptr, std::move(view), std::move(controller));
}

} // namespace scene::load_game
