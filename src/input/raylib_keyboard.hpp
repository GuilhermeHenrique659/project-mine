#pragma once

#include "input/keyboard.hpp"

namespace input {

class RaylibKeyboard : public Keyboard {
public:
    bool is_pressed(Key key) const override;
};

} // namespace input
