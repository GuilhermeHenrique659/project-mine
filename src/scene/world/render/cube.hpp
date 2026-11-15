#pragma once

#include "scene/world/core/isometric.hpp"
#include "ui/canvas.hpp"
#include "ui/geometry.hpp"

namespace scene::world {

// Side of a building's cube at zoom 1: as wide as a tile.
inline constexpr float BuildingCubeSize = 64.0f;

// A cube seen from the front: a lighter top face over a square front face. `size` is the
// side of the front face in pixels, so any size can be created. The front face is centred
// on the position it is drawn at; the top face sits right above it.
class Cube {
public:
    explicit Cube(float size, ui::Color front = {200, 160, 50, 255},
                  ui::Color top = {240, 210, 110, 255})
        : size_(size), front_(front), top_(top) {}

    // Everything the cube covers when drawn at `center`.
    ui::Rect bounds(::world::ScreenPoint center) const;

    void draw(ui::Canvas& canvas, ::world::ScreenPoint center) const;

private:
    float size_;
    ui::Color front_;
    ui::Color top_;
};

} // namespace scene::world
