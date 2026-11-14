#include "scene/world/world_scene.hpp"

#include <raylib.h>

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/input_handler.hpp"
#include "input/raylib_keyboard.hpp"
#include "scene/world/world_model.hpp"
#include "scene/world/world_view.hpp"
#include "world/isometric.hpp"

namespace scene::world {

namespace {

// Translates player input into calls on the view or business events. It only knows
// actions (see input::Action), never physical keys.
class WorldController : public scene::Controller {
public:
    WorldController(WorldView& view, WorldModel& model)
        : view_(view), model_(model), input_(keyboard_, input::InputMap::world_defaults()) {
        ui::FocusGroup& focus = view_.menu().focus_group();
        // The menu is hidden while playing, so keyboard navigation only applies when paused.
        input_.on(input::Action::NavigateUp, [&focus, &model] {
            if (model.is_paused()) focus.focus_previous();
        });
        input_.on(input::Action::NavigateDown, [&focus, &model] {
            if (model.is_paused()) focus.focus_next();
        });
        input_.on(input::Action::Confirm, [&focus, &model] {
            if (model.is_paused()) focus.activate_focused();
        });
        input_.on(input::Action::Pause, [&model] { model.toggle_pause(); });
    }

    void handle_input() override {
        const Vector2 mouse = GetMousePosition();
        view_.handle_pointer({mouse.x, mouse.y}, IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
        input_.update();
        model_.set_player_direction(::world::screen_direction_to_world(held_direction()));
    }

private:
    // Movement the player asks for, in screen space (y grows downwards).
    ::world::ScreenPoint held_direction() const {
        ::world::ScreenPoint direction;
        if (input_.is_held(input::Action::MoveUp)) direction.y -= 1.0f;
        if (input_.is_held(input::Action::MoveDown)) direction.y += 1.0f;
        if (input_.is_held(input::Action::MoveLeft)) direction.x -= 1.0f;
        if (input_.is_held(input::Action::MoveRight)) direction.x += 1.0f;
        return direction;
    }

    WorldView& view_;
    WorldModel& model_;
    input::RaylibKeyboard keyboard_; // declared before input_, which references it
    input::InputHandler input_;
};

} // namespace

std::unique_ptr<scene::Scene> make_world_scene(game::Game& game, game::SaveGame save) {
    auto model = std::make_unique<WorldModel>(game.event_bus(), std::move(save));
    auto view = std::make_unique<WorldView>(game.event_bus(), *model);
    auto controller = std::make_unique<WorldController>(*view, *model);
    return std::make_unique<scene::Scene>(std::move(model), std::move(view),
                                          std::move(controller));
}

} // namespace scene::world
