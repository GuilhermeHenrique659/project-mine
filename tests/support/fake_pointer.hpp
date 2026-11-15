#pragma once

#include "input/pointer.hpp"

// Pointer whose state is set by the test: `at` is where it is, `release` is true on the
// frame the button was released (a click), `held` is true while it is down.
struct FakePointer : input::Pointer {
    input::PointerPosition position() const override { return at; }
    bool released() const override { return release; }
    bool down() const override { return held; }
    float scroll() const override { return wheel; }

    input::PointerPosition at;
    bool release = false;
    bool held = false;
    float wheel = 0.0f;
};
