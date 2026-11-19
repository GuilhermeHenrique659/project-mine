#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "input/keyboard.hpp"
#include "input/pointer.hpp"
#include "scene/new_game/new_game_view.hpp"
#include "scene/panel_view.hpp"

namespace scene::new_game {

// The New Game screen's controller: a PanelController on the text entry bindings (letters
// and Space type into the name), plus Erase (Backspace) and the typed characters, which go
// to the view; Cancel goes back to the main menu.
class NewGameController : public scene::PanelController {
public:
    NewGameController(NewGameView& view, event::EventBus& bus,
                      std::unique_ptr<input::Keyboard> keyboard,
                      std::shared_ptr<const input::Pointer> pointer);

    void handle_input() override;

    // The keyboard part of handle_input(): typed characters, then the actions.
    void handle_keyboard();

private:
    NewGameView& view_;
};

} // namespace scene::new_game
