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
        case Key::Backspace: return KEY_BACKSPACE;
        case Key::W: return KEY_W;
        case Key::A: return KEY_A;
        case Key::S: return KEY_S;
        case Key::D: return KEY_D;
        case Key::LeftShift: return KEY_LEFT_SHIFT;
        case Key::RightShift: return KEY_RIGHT_SHIFT;
        case Key::Tab: return KEY_TAB;
        case Key::B: return KEY_B;
    }
    return KEY_NULL;
}

} // namespace

bool RaylibKeyboard::is_pressed(Key key) const {
    const int code = to_raylib(key);
    if (IsKeyPressed(code)) {
        return true;
    }
    // Holding Backspace keeps erasing, like any text field.
    return key == Key::Backspace && IsKeyPressedRepeat(code);
}

bool RaylibKeyboard::is_down(Key key) const { return IsKeyDown(to_raylib(key)); }

std::optional<char> RaylibKeyboard::next_char() {
    // Only printable ASCII is supported by ui::Input; other code points are dropped.
    for (int code = GetCharPressed(); code != 0; code = GetCharPressed()) {
        if (code >= 32 && code < 127) {
            return static_cast<char>(code);
        }
    }
    return std::nullopt;
}

} // namespace input
