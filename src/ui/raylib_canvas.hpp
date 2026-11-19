#pragma once

#include "ui/canvas.hpp"

namespace ui {

// Canvas backed by raylib. Only usable between BeginDrawing/EndDrawing.
class RaylibCanvas : public Canvas {
public:
    void fill_rect(Rect rect, Color color) override;
    void fill_triangle(Point a, Point b, Point c, Color color) override;
    void stroke_rect(Rect rect, Color color, float thickness) override;
    void draw_text(const std::string& text, Point position, int font_size, Color color) override;
    using Canvas::draw_texture;
    void draw_texture(const graphics::Texture& texture, Rect dest,
                      const TextureDraw& options) override;
    Size measure_text(const std::string& text, int font_size) const override;

    // Size of the raylib window, in pixels.
    static Size screen_size();

protected:
    void apply_clip(const std::optional<Rect>& clip) override;
};

} // namespace ui
