#pragma once

#include <cstdint>

namespace ui {

struct Point {
    float x = 0.0f;
    float y = 0.0f;
};

struct Size {
    float width = 0.0f;
    float height = 0.0f;
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool contains(Point point) const {
        return point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;
    }
};

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    bool operator==(const Color&) const = default;
};

// Space around content, like CSS padding/margin.
struct Insets {
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;

    static constexpr Insets all(float value) { return {value, value, value, value}; }
    static constexpr Insets symmetric(float vertical, float horizontal) {
        return {vertical, horizontal, vertical, horizontal};
    }
    constexpr float horizontal() const { return left + right; }
    constexpr float vertical() const { return top + bottom; }
};

} // namespace ui
