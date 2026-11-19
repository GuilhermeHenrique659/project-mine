#include "scene/new_game/new_game_controller.hpp"

#include "game/game_events.hpp"

namespace scene::new_game {

NewGameController::NewGameController(NewGameView& view, event::EventBus& bus,
                                     std::unique_ptr<input::Keyboard> keyboard,
                                     std::shared_ptr<const input::Pointer> pointer)
    : PanelController(view, std::move(keyboard), input::InputMap::text_entry_defaults(),
                      std::move(pointer)),
      view_(view) {
    input().on(input::Action::Erase, [this] { view_.erase_last(); });
    on_cancel([&bus] { bus.publish(game::BackToMenuRequested{}); });
}

void NewGameController::handle_input() {
    handle_pointer();
    handle_keyboard();
}

void NewGameController::handle_keyboard() {
    while (const auto character = keyboard().next_char()) {
        view_.type_char(*character);
    }
    input().update();
}

} // namespace scene::new_game
