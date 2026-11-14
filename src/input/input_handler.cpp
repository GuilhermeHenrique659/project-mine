#include "input/input_handler.hpp"

#include <algorithm>
#include <vector>

namespace input {

void InputHandler::update() {
    std::vector<Action> triggered;
    for (const Binding& binding : map_.bindings()) {
        if (!keyboard_.is_pressed(binding.key)) {
            continue;
        }
        if (std::find(triggered.begin(), triggered.end(), binding.action) != triggered.end()) {
            continue;
        }
        triggered.push_back(binding.action);
    }

    for (const Action action : triggered) {
        const auto handler = handlers_.find(action);
        if (handler == handlers_.end() || !handler->second) {
            continue;
        }
        handler->second();
    }
}

} // namespace input
