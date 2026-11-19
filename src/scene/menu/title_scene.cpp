#include "scene/menu/title_scene.hpp"

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/raylib_keyboard.hpp"
#include "input/raylib_pointer.hpp"
#include "scene/menu/menu_view.hpp"

namespace scene::menu {

std::unique_ptr<scene::Scene> make_title_scene(game::Game& game) {
    // The menu has no state of its own, so the scene has no model.
    auto view = std::make_unique<MenuView>(game.event_bus());
    auto controller = std::make_unique<scene::PanelController>(
        *view, std::make_unique<input::RaylibKeyboard>(), input::InputMap::menu_defaults(),
        std::make_shared<input::RaylibPointer>());
    controller->on_cancel([&game] { game.event_bus().publish(game::QuitRequested{}); });
    return std::make_unique<scene::Scene>(nullptr, std::move(view), std::move(controller));
}

} // namespace scene::menu
