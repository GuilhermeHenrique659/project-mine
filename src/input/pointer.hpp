#pragma once

namespace input {

// Position of the pointer in screen space (pixels, y grows downwards).
struct PointerPosition {
    float x = 0.0f;
    float y = 0.0f;
};

// Source of mouse state. Implemented on top of raylib for the game and faked in tests.
class Pointer {
public:
    virtual ~Pointer() = default;
    virtual PointerPosition position() const = 0;
    // True only on the frame the main (left) button was released: a click.
    virtual bool released() const = 0;
    // Mouse wheel movement this frame in notches: positive = away from the user (up).
    virtual float scroll() const = 0;
};

} // namespace input
