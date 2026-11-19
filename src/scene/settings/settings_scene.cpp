#include "scene/settings/settings_scene.hpp"

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/raylib_keyboard.hpp"
#include "input/raylib_pointer.hpp"
#include "scene/settings/settings_view.hpp"

namespace scene::settings {

std::unique_ptr<scene::Scene> make_settings_scene(game::Game& game) {
    // The screen keeps no state beyond the view, so the scene has no model.
    auto view = std::make_unique<SettingsView>(game.event_bus(), game.display_settings());
    auto controller = std::make_unique<scene::PanelController>(
        *view, std::make_unique<input::RaylibKeyboard>(), input::InputMap::menu_defaults(),
        std::make_shared<input::RaylibPointer>());
    controller->on_cancel([&game] { game.event_bus().publish(game::BackToMenuRequested{}); });
    return std::make_unique<scene::Scene>(nullptr, std::move(view), std::move(controller));
}

} // namespace scene::settings
