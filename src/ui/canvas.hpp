#pragma once

#include <string>

#include "ui/geometry.hpp"

namespace ui {

// Drawing surface used by widgets. Keeps widgets independent from raylib so
// layout and drawing decisions can be tested with a fake canvas.
class Canvas {
public:
    virtual ~Canvas() = default;
    virtual void fill_rect(Rect rect, Color color) = 0;
    virtual void stroke_rect(Rect rect, Color color, float thickness) = 0;
    virtual void draw_text(const std::string& text, Point position, int font_size, Color color) = 0;
    virtual Size measure_text(const std::string& text, int font_size) const = 0;
};

} // namespace ui
