#pragma once

#include "ui/canvas.hpp"

namespace ui {

// Canvas backed by raylib. Only usable between BeginDrawing/EndDrawing.
class RaylibCanvas : public Canvas {
public:
    void fill_rect(Rect rect, Color color) override;
    void stroke_rect(Rect rect, Color color, float thickness) override;
    void draw_text(const std::string& text, Point position, int font_size, Color color) override;
    Size measure_text(const std::string& text, int font_size) const override;
};

} // namespace ui
