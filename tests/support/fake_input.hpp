#pragma once

#include "input/input_handler.hpp"
#include "input/input_map.hpp"
#include "support/fake_keyboard.hpp"

// A world-bindings input handler over a FakeKeyboard, to inject into the world model or
// its systems. Set `keyboard.down` to hold keys.
struct FakeInput {
    FakeKeyboard keyboard;
    input::InputHandler handler{keyboard, input::InputMap::world_defaults()};
};
