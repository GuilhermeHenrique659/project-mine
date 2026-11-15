#include "input/raylib_pointer.hpp"

#include <raylib.h>

namespace input {

PointerPosition RaylibPointer::position() const {
    const Vector2 mouse = GetMousePosition();
    return {mouse.x, mouse.y};
}

bool RaylibPointer::released() const { return IsMouseButtonReleased(MOUSE_BUTTON_LEFT); }

float RaylibPointer::scroll() const { return GetMouseWheelMove(); }

} // namespace input
