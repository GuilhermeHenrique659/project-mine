#include "scene/load_game/load_game_scene.hpp"

#include <raylib.h>

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/input_handler.hpp"
#include "input/raylib_keyboard.hpp"
#include "scene/load_game/load_game_view.hpp"

namespace scene::load_game {

namespace {

// Translates player input into calls on the view or business events. It only knows
// actions (see input::Action), never physical keys.
class LoadGameController : public scene::Controller {
public:
    LoadGameController(LoadGameView& view, game::Game& game)
        : view_(view), input_(keyboard_, input::InputMap::menu_defaults()) {
        ui::FocusGroup& focus = view_.focus_group();
        input_.on(input::Action::NavigateUp, [&focus] { focus.focus_previous(); });
        input_.on(input::Action::NavigateDown, [&focus] { focus.focus_next(); });
        input_.on(input::Action::Confirm, [&focus] { focus.activate_focused(); });
        input_.on(input::Action::Cancel,
                  [&game] { game.event_bus().publish(game::BackToMenuRequested{}); });
    }

    void handle_input() override {
        const Vector2 mouse = GetMousePosition();
        view_.handle_pointer({mouse.x, mouse.y}, IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
        input_.update();
    }

private:
    LoadGameView& view_;
    input::RaylibKeyboard keyboard_; // declared before input_, which references it
    input::InputHandler input_;
};

} // namespace

std::unique_ptr<scene::Scene> make_load_game_scene(game::Game& game) {
    // The list is read once, when the screen opens; the scene keeps no state of its own.
    auto view = std::make_unique<LoadGameView>(game.event_bus(), game.save_store().list());
    auto controller = std::make_unique<LoadGameController>(*view, game);
    return std::make_unique<scene::Scene>(nullptr, std::move(view), std::move(controller));
}

} // namespace scene::load_game
