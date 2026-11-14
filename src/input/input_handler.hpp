#pragma once

#include <functional>
#include <map>

#include "input/action.hpp"
#include "input/input_map.hpp"
#include "input/keyboard.hpp"

namespace input {

// Turns keyboard state into actions. A scene registers what each action does and calls
// update() once per frame; it never names a physical key:
//   handler.on(Action::Confirm, [&] { focus.activate_focused(); });
//   handler.update();
// The keyboard must outlive the handler.
class InputHandler {
public:
    using Handler = std::function<void()>;

    InputHandler(const Keyboard& keyboard, InputMap map)
        : keyboard_(keyboard), map_(std::move(map)) {}

    // Registers the handler of an action, replacing any previous one.
    void on(Action action, Handler handler) { handlers_[action] = std::move(handler); }

    // Polls the keyboard and runs the handler of every action triggered this frame
    // (once per action, even if several of its keys were pressed).
    void update();

    // True while any key bound to the action is held down (for continuous input such as
    // walking, as opposed to the once-per-press handlers).
    bool is_held(Action action) const;

    InputMap& map() { return map_; }

private:
    const Keyboard& keyboard_;
    InputMap map_;
    std::map<Action, Handler> handlers_;
};

} // namespace input
