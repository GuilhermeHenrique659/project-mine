#include "scene/world/world_scene.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "game/game.hpp"
#include "game/game_events.hpp"
#include "input/input_handler.hpp"
#include "input/pointer_gate.hpp"
#include "input/raylib_keyboard.hpp"
#include "input/raylib_pointer.hpp"
#include "scene/world/render/entity_renderer.hpp"
#include "scene/world/render/player_hud.hpp"
#include "scene/world/render/player_menu.hpp"
#include "scene/world/render/site_work.hpp"
#include "scene/world/render/unit_command.hpp"
#include "scene/world/render/terrain_renderer.hpp"
#include "scene/world/render/building_panel.hpp"
#include "scene/world/render/world_menu.hpp"
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
// menu renderer reads it), except the wheel, which zooms the camera.
class WorldController : public scene::Controller {
public:
    WorldController(WorldMenu& menu, WorldModel& model, std::shared_ptr<WorldInput> input,
                    std::shared_ptr<input::Pointer> pointer)
        : input_(std::move(input)), model_(model), pointer_(std::move(pointer)) {
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
        // Esc closes the player menu first, otherwise opens or closes the pause.
        input_->handler.on(input::Action::Pause, [&model] { model.cancel(); });
        input_->handler.on(input::Action::ToggleInventory,
                           [&model] { model.toggle_player_menu(); });
        input_->handler.on(input::Action::ToggleBuildMenu, [&model] { model.toggle_build_bar(); });
    }

    // The mouse is not handled here: the menu renderer reads it itself.
    void handle_input() override {
        input_->handler.update();
        model_.zoom_camera(pointer_->scroll());
    }

private:
    std::shared_ptr<WorldInput> input_;
    WorldModel& model_;
    std::shared_ptr<input::Pointer> pointer_;
};

} // namespace

std::unique_ptr<scene::Scene> make_world_scene(game::Game& game, game::SaveGame save) {
    auto input = std::make_shared<WorldInput>();
    auto model = std::make_unique<WorldModel>(game.event_bus(), input->handler, std::move(save));
    std::shared_ptr<input::Pointer> mouse = std::make_shared<input::RaylibPointer>();
    // The renderers read the mouse through the gate, so a panel on top keeps clicks from
    // reaching the ones below it (the world, the build bar).
    auto pointer = std::make_shared<input::PointerGate>(mouse);

    // Bottom to top. Adding something to the screen is one more entry here.
    auto menu = std::make_unique<WorldMenu>(game.event_bus(), *model, pointer);
    WorldMenu& menu_ref = *menu; // owned by the view, which outlives the controller
    std::vector<std::unique_ptr<Renderer>> renderers;
    renderers.push_back(std::make_unique<TerrainRenderer>(model->terrain(), model->camera()));
    renderers.push_back(std::make_unique<EntityRenderer>(model->registry(), model->camera()));
    renderers.push_back(std::make_unique<SiteWork>(*model, pointer));
    renderers.push_back(std::make_unique<UnitCommand>(*model, pointer));
    renderers.push_back(std::make_unique<PlayerHud>(*model, pointer));
    renderers.push_back(std::make_unique<BuildingPanel>(*model, pointer));
    renderers.push_back(std::make_unique<PlayerMenu>(*model, pointer));
    renderers.push_back(std::move(menu));

    auto view = std::make_unique<WorldView>(std::move(renderers), pointer);
    auto controller = std::make_unique<WorldController>(menu_ref, *model, std::move(input), mouse);
    return std::make_unique<scene::Scene>(std::move(model), std::move(view),
                                          std::move(controller));
}

} // namespace scene::world
