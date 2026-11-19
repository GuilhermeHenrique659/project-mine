#include "input/raylib_pointer.hpp"

#include <raylib.h>

namespace input {

PointerPosition RaylibPointer::position() const {
    const Vector2 mouse = GetMousePosition();
    return {mouse.x, mouse.y};
}

bool RaylibPointer::released() const { return IsMouseButtonReleased(MOUSE_BUTTON_LEFT); }

bool RaylibPointer::down() const { return IsMouseButtonDown(MOUSE_BUTTON_LEFT); }

bool RaylibPointer::secondary_down() const { return IsMouseButtonDown(MOUSE_BUTTON_RIGHT); }

bool RaylibPointer::secondary_released() const {
    return IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
}

float RaylibPointer::scroll() const { return GetMouseWheelMove(); }

} // namespace input
