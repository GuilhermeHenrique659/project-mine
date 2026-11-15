#pragma once

#include <array>

#include "scene/world/core/isometric.hpp"
#include "ui/canvas.hpp"
#include "ui/geometry.hpp"

namespace scene::world {

// Side of a building's cube at zoom 1: as wide as a tile.
inline constexpr float BuildingCubeSize = 64.0f;

// The three visible faces of an isometric cube, each as four screen corners.
// top: top, right, bottom, left (a 2:1 diamond, like a tile).
// left / right: top-outer, top-front, bottom-front, bottom-outer.
struct CubeFaces {
    std::array<::world::ScreenPoint, 4> top;
    std::array<::world::ScreenPoint, 4> left;
    std::array<::world::ScreenPoint, 4> right;
};

// A cube in the same 2:1 isometric projection as the tiles: `size` is its width on screen
// (its base is a diamond `size` wide and `size` / 2 tall), standing on the ground point it
// is drawn at (the centre of the base). The top face has the `top` colour, the left face
// `front` and the right face a darker `front`, as if lit from above-left.
class Cube {
public:
    explicit Cube(float size, ui::Color front = {200, 160, 50, 255},
                  ui::Color top = {240, 210, 110, 255})
        : size_(size), front_(front), top_(top) {}

    // How tall its vertical edges are on screen.
    float height() const;

    CubeFaces faces(::world::ScreenPoint ground) const;

    // Everything the cube covers when drawn at `ground`.
    ui::Rect bounds(::world::ScreenPoint ground) const;

    void draw(ui::Canvas& canvas, ::world::ScreenPoint ground) const;

private:
    float size_;
    ui::Color front_;
    ui::Color top_;
};

} // namespace scene::world
