#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>

#include "input/keyboard.hpp"

// Keyboard whose state is set by the test: `pressed` holds the keys that went down this
// frame, `typed` the characters still to be read.
struct FakeKeyboard : input::Keyboard {
    bool is_pressed(input::Key key) const override { return pressed.contains(key); }

    std::optional<char> next_char() override {
        if (typed.empty()) {
            return std::nullopt;
        }
        const char character = typed.front();
        typed.pop_front();
        return character;
    }

    void type(const std::string& text) { typed.insert(typed.end(), text.begin(), text.end()); }

    std::set<input::Key> pressed;
    std::deque<char> typed;
};
