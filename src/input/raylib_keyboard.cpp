#include "input/raylib_keyboard.hpp"

#include <raylib.h>

namespace input {

namespace {

int to_raylib(Key key) {
    switch (key) {
        case Key::Up: return KEY_UP;
        case Key::Down: return KEY_DOWN;
        case Key::Left: return KEY_LEFT;
        case Key::Right: return KEY_RIGHT;
        case Key::Enter: return KEY_ENTER;
        case Key::Escape: return KEY_ESCAPE;
        case Key::Space: return KEY_SPACE;
        case Key::W: return KEY_W;
        case Key::A: return KEY_A;
        case Key::S: return KEY_S;
        case Key::D: return KEY_D;
    }
    return KEY_NULL;
}

} // namespace

bool RaylibKeyboard::is_pressed(Key key) const { return IsKeyPressed(to_raylib(key)); }

} // namespace input
