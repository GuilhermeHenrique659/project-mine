#pragma once

#include <optional>

#include "input/key.hpp"

namespace input {

// Source of keyboard state. Implemented on top of raylib for the game and faked in tests.
class Keyboard {
public:
    virtual ~Keyboard() = default;
    // True only on the frame the key went down.
    virtual bool is_pressed(Key key) const = 0;
    // True every frame the key is held, including the frame it went down.
    virtual bool is_down(Key key) const = 0;
    // Next character typed this frame, in order; empty when there is none left. Used by
    // text fields, which need the character and not the physical key.
    virtual std::optional<char> next_char() { return std::nullopt; }
};

} // namespace input
