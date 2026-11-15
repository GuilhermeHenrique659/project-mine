#include "input/input_map.hpp"

#include <algorithm>

namespace input {

InputMap& InputMap::bind(Key key, Action action) {
    const auto existing = std::find_if(bindings_.begin(), bindings_.end(),
                                       [key](const Binding& binding) { return binding.key == key; });
    if (existing != bindings_.end()) {
        existing->action = action;
        return *this;
    }
    bindings_.push_back({key, action});
    return *this;
}

std::optional<Action> InputMap::action_for(Key key) const {
    const auto found = std::find_if(bindings_.begin(), bindings_.end(),
                                    [key](const Binding& binding) { return binding.key == key; });
    if (found == bindings_.end()) {
        return std::nullopt;
    }
    return found->action;
}

InputMap InputMap::menu_defaults() {
    InputMap map;
    map.bind(Key::Up, Action::NavigateUp)
        .bind(Key::W, Action::NavigateUp)
        .bind(Key::Down, Action::NavigateDown)
        .bind(Key::S, Action::NavigateDown)
        .bind(Key::Left, Action::NavigateLeft)
        .bind(Key::A, Action::NavigateLeft)
        .bind(Key::Right, Action::NavigateRight)
        .bind(Key::D, Action::NavigateRight)
        .bind(Key::Enter, Action::Confirm)
        .bind(Key::Space, Action::Confirm)
        .bind(Key::Escape, Action::Cancel);
    return map;
}

InputMap InputMap::world_defaults() {
    InputMap map;
    map.bind(Key::W, Action::MoveUp)
        .bind(Key::S, Action::MoveDown)
        .bind(Key::A, Action::MoveLeft)
        .bind(Key::D, Action::MoveRight)
        .bind(Key::LeftShift, Action::Run)
        .bind(Key::RightShift, Action::Run)
        .bind(Key::Up, Action::NavigateUp)
        .bind(Key::Down, Action::NavigateDown)
        .bind(Key::Left, Action::NavigateLeft)
        .bind(Key::Right, Action::NavigateRight)
        .bind(Key::Enter, Action::Confirm)
        .bind(Key::Space, Action::Confirm)
        .bind(Key::Escape, Action::Pause)
        .bind(Key::Tab, Action::ToggleInventory);
    return map;
}

InputMap InputMap::text_entry_defaults() {
    InputMap map;
    map.bind(Key::Up, Action::NavigateUp)
        .bind(Key::Down, Action::NavigateDown)
        .bind(Key::Enter, Action::Confirm)
        .bind(Key::Escape, Action::Cancel)
        .bind(Key::Backspace, Action::Erase);
    return map;
}

} // namespace input
