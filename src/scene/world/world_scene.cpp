#include "scene/world/world_scene.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/input_handler.hpp"
#include "input/raylib_keyboard.hpp"
#include "input/raylib_pointer.hpp"
#include "scene/world/entity_renderer.hpp"
#include "scene/world/world_menu.hpp"
#include "scene/world/world_model.hpp"
#include "scene/world/world_view.hpp"

namespace scene::world {

namespace {

// The keyboard and the action handler of the world screen. The model's input system
// reads the handler while the controller registers the menu actions on it, so both are
// created first and shared.
struct WorldInput {
    input::RaylibKeyboard keyboard; // declared before handler, which references it
    input::InputHandler handler{keyboard, input::InputMap::world_defaults()};
};

// Translates player input into calls on the view or business events. It only knows
// actions (see input::Action), never physical keys. Movement is not handled here (the
// model's PlayerInputSystem reads the held move actions) and neither is the mouse (the
// menu renderer reads it).
class WorldController : public scene::Controller {
public:
    WorldController(WorldMenu& menu, WorldModel& model, std::shared_ptr<WorldInput> input)
        : input_(std::move(input)) {
        ui::FocusGroup& focus = menu.focus_group();
        // The menu is hidden while playing, so keyboard navigation only applies when paused.
        input_->handler.on(input::Action::NavigateUp, [&focus, &model] {
            if (model.is_paused()) focus.focus_previous();
        });
        input_->handler.on(input::Action::NavigateDown, [&focus, &model] {
            if (model.is_paused()) focus.focus_next();
        });
        input_->handler.on(input::Action::Confirm, [&focus, &model] {
            if (model.is_paused()) focus.activate_focused();
        });
        input_->handler.on(input::Action::Pause, [&model] { model.toggle_pause(); });
    }

    // The mouse is not handled here: the menu renderer reads it itself.
    void handle_input() override { input_->handler.update(); }

private:
    std::shared_ptr<WorldInput> input_;
};

} // namespace

std::unique_ptr<scene::Scene> make_world_scene(game::Game& game, game::SaveGame save) {
    auto input = std::make_shared<WorldInput>();
    auto model = std::make_unique<WorldModel>(game.event_bus(), input->handler, std::move(save));
    auto pointer = std::make_shared<input::RaylibPointer>();

    // Bottom to top. Adding something to the screen is one more entry here.
    auto menu = std::make_unique<WorldMenu>(game.event_bus(), *model, std::move(pointer));
    WorldMenu& menu_ref = *menu; // owned by the view, which outlives the controller
    std::vector<std::unique_ptr<Renderer>> renderers;
    renderers.push_back(std::make_unique<EntityRenderer>(model->registry()));
    renderers.push_back(std::move(menu));

    auto view = std::make_unique<WorldView>(std::move(renderers));
    auto controller = std::make_unique<WorldController>(menu_ref, *model, std::move(input));
    return std::make_unique<scene::Scene>(std::move(model), std::move(view),
                                          std::move(controller));
}

} // namespace scene::world
