#pragma once

#include "input/keyboard.hpp"

namespace input {

class RaylibKeyboard : public Keyboard {
public:
    bool is_pressed(Key key) const override;
    bool is_down(Key key) const override;
    std::optional<char> next_char() override;
};

} // namespace input
