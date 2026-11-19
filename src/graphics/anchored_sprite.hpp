#pragma once

#include "graphics/sprite.hpp"
#include "ui/geometry.hpp"

namespace graphics {

// A sprite that stands on a ground point (a building, a character): its size in pixels at
// the default zoom and the pixel of it that sits on that point.
struct AnchoredSprite {
    Sprite sprite;
    ui::Size size;
    ui::Point anchor;

    // Where to draw it so the anchor lands on `ground`, at `scale` times the default zoom.
    ui::Rect dest(ui::Point ground, float scale) const {
        return {ground.x - anchor.x * scale, ground.y - anchor.y * scale, size.width * scale,
                size.height * scale};
    }
};

} // namespace graphics
