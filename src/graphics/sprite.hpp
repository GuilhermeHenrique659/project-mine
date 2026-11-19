#pragma once

#include <optional>
#include <string>

#include "ui/canvas.hpp"

namespace graphics {

// A picture to draw: a registered texture (see TextureManager) and, for a sprite sheet
// frame, the part of it to use. Without `source` the whole texture is the sprite.
struct Sprite {
    std::string texture;
    std::optional<ui::Rect> source;
};

// Draws `sprite` over `dest`. `options.source` is replaced by the sprite's own region.
// A texture that does not exist is replaced by the (whole) fallback texture. Returns false,
// drawing nothing, only when there is no fallback either.
bool draw_sprite(ui::Canvas& canvas, const Sprite& sprite, ui::Rect dest,
                 ui::TextureDraw options = {});

// Like draw_sprite(), but a texture that does not exist draws nothing (no fallback), so
// the caller can keep what is already under it. Returns whether the sprite was drawn.
bool draw_loaded_sprite(ui::Canvas& canvas, const Sprite& sprite, ui::Rect dest,
                        ui::TextureDraw options = {});

} // namespace graphics
