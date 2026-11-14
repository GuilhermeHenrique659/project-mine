#include "scene/new_game/new_game_controller.hpp"

#include <raylib.h>

#include "game/game_events.hpp"

namespace scene::new_game {

NewGameController::NewGameController(NewGameView& view, event::EventBus& bus,
                                     std::unique_ptr<input::Keyboard> keyboard)
    : view_(view),
      keyboard_(std::move(keyboard)),
      input_(*keyboard_, input::InputMap::text_entry_defaults()) {
    ui::FocusGroup& focus = view_.focus_group();
    input_.on(input::Action::NavigateUp, [&focus] { focus.focus_previous(); });
    input_.on(input::Action::NavigateDown, [&focus] { focus.focus_next(); });
    input_.on(input::Action::Confirm, [&focus] { focus.activate_focused(); });
    input_.on(input::Action::Erase, [this] { view_.erase_last(); });
    input_.on(input::Action::Cancel, [&bus] { bus.publish(game::BackToMenuRequested{}); });
}

void NewGameController::handle_input() {
    const Vector2 mouse = GetMousePosition();
    view_.handle_pointer({mouse.x, mouse.y}, IsMouseButtonReleased(MOUSE_BUTTON_LEFT));
    handle_keyboard();
}

void NewGameController::handle_keyboard() {
    while (const auto character = keyboard_->next_char()) {
        view_.type_char(*character);
    }
    input_.update();
}

} // namespace scene::new_game
