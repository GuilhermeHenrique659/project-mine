#pragma once

#include "input/pointer.hpp"

// Pointer whose state is set by the test: `at` is where it is, `release` is true on the
// frame the button was released (a click), `held` is true while it is down; `right_held`
// and `right_release` are the same for the right button.
struct FakePointer : input::Pointer {
    input::PointerPosition position() const override { return at; }
    bool released() const override { return release; }
    bool down() const override { return held; }
    bool secondary_down() const override { return right_held; }
    bool secondary_released() const override { return right_release; }
    float scroll() const override { return wheel; }

    input::PointerPosition at;
    bool release = false;
    bool held = false;
    bool right_held = false;
    bool right_release = false;
    float wheel = 0.0f;
};
