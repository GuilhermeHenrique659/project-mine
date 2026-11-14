#pragma once

#include <optional>
#include <vector>

#include "input/action.hpp"
#include "input/key.hpp"

namespace input {

struct Binding {
    Key key;
    Action action;
};

// Which key triggers which action. Several keys may trigger the same action, a key
// triggers one action; binding a key again moves it to the new action (rebinding).
class InputMap {
public:
    InputMap& bind(Key key, Action action);
    std::optional<Action> action_for(Key key) const;
    const std::vector<Binding>& bindings() const { return bindings_; }

    // Arrows/WASD navigate, Enter/Space confirm, Esc cancels.
    static InputMap menu_defaults();
    // In-game: WASD moves, arrows navigate the panel, Enter/Space confirm, Esc cancels.
    static InputMap world_defaults();
    // For screens with a text field: arrows navigate, Enter confirms, Esc cancels and
    // Backspace erases. Letters and Space stay unbound so they can be typed.
    static InputMap text_entry_defaults();

private:
    std::vector<Binding> bindings_;
};

} // namespace input
