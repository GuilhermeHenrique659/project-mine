#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "input/input_handler.hpp"
#include "input/keyboard.hpp"
#include "scene/new_game/new_game_view.hpp"
#include "scene/scene.hpp"

namespace scene::new_game {

// Translates player input into calls on the view or business events. It only knows
// actions (see input::Action) and typed characters, never physical keys.
class NewGameController : public scene::Controller {
public:
    NewGameController(NewGameView& view, event::EventBus& bus,
                      std::unique_ptr<input::Keyboard> keyboard);

    // Mouse (read from raylib) and then the keyboard.
    void handle_input() override;

    // Typed characters go to the name field; actions navigate, submit, erase or go back.
    void handle_keyboard();

private:
    NewGameView& view_;
    std::unique_ptr<input::Keyboard> keyboard_; // declared before input_, which references it
    input::InputHandler input_;
};

} // namespace scene::new_game
