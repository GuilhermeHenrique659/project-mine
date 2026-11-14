#pragma once

#include <optional>
#include <string>

#include "graphics/texture.hpp"
#include "ui/geometry.hpp"

namespace ui {

// How a texture is drawn. The defaults draw the whole texture as it is.
struct TextureDraw {
    // Part of the texture to draw, in texture pixels (a sprite sheet frame). Whole if empty.
    std::optional<Rect> source;
    Color tint{255, 255, 255, 255};
    bool flip_x = false;
    bool flip_y = false;
    // Degrees clockwise, around the centre of the destination.
    float rotation = 0.0f;
};

// Drawing surface used by widgets. Keeps widgets independent from raylib so
// layout and drawing decisions can be tested with a fake canvas.
class Canvas {
public:
    virtual ~Canvas() = default;
    virtual void fill_rect(Rect rect, Color color) = 0;
    virtual void stroke_rect(Rect rect, Color color, float thickness) = 0;
    virtual void draw_text(const std::string& text, Point position, int font_size, Color color) = 0;
    // Draws the texture (or `options.source` of it) stretched over `dest`.
    virtual void draw_texture(const graphics::Texture& texture, Rect dest,
                              const TextureDraw& options) = 0;
    void draw_texture(const graphics::Texture& texture, Rect dest) {
        draw_texture(texture, dest, TextureDraw{});
    }
    virtual Size measure_text(const std::string& text, int font_size) const = 0;
};

} // namespace ui
