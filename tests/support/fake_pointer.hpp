#pragma once

#include "input/pointer.hpp"

// Pointer whose state is set by the test: `at` is where it is, `release` is true on the
// frame the button was released (a click).
struct FakePointer : input::Pointer {
    input::PointerPosition position() const override { return at; }
    bool released() const override { return release; }

    input::PointerPosition at;
    bool release = false;
};
