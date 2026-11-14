#include "ui/raylib_canvas.hpp"

#include <raylib.h>

namespace ui {

namespace {
::Color to_raylib(Color color) { return {color.r, color.g, color.b, color.a}; }
} // namespace

void RaylibCanvas::fill_rect(Rect rect, Color color) {
    DrawRectangleRec({rect.x, rect.y, rect.width, rect.height}, to_raylib(color));
}

void RaylibCanvas::stroke_rect(Rect rect, Color color, float thickness) {
    DrawRectangleLinesEx({rect.x, rect.y, rect.width, rect.height}, thickness, to_raylib(color));
}

void RaylibCanvas::draw_text(const std::string& text, Point position, int font_size, Color color) {
    DrawText(text.c_str(), static_cast<int>(position.x), static_cast<int>(position.y), font_size,
             to_raylib(color));
}

Size RaylibCanvas::measure_text(const std::string& text, int font_size) const {
    return {static_cast<float>(MeasureText(text.c_str(), font_size)),
            static_cast<float>(font_size)};
}

} // namespace ui
