#include "scene/world/world_scene.hpp"

#include <raylib.h>

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/input_handler.hpp"
#include "input/raylib_keyboard.hpp"
#include "scene/world/world_model.hpp"
#include "scene/world/world_view.hpp"

namespace scene::world {

namespace {

// Translates player input into calls on the view or business events. It only knows
// actions (see input::Action), never physical keys.
class WorldController : public scene::Controller {
public:
    WorldController(WorldView& view, game::Game& game)
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
    WorldView& view_;
    input::RaylibKeyboard keyboard_; // declared before input_, which references it
    input::InputHandler input_;
};

} // namespace

std::unique_ptr<scene::Scene> make_world_scene(game::Game& game, game::SaveGame save) {
    auto model = std::make_unique<WorldModel>(std::move(save));
    auto view = std::make_unique<WorldView>(game.event_bus(), *model);
    auto controller = std::make_unique<WorldController>(*view, game);
    return std::make_unique<scene::Scene>(std::move(model), std::move(view),
                                          std::move(controller));
}

} // namespace scene::world
