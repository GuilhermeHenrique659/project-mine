#pragma once

#include <array>

#include "scene/world/core/components/building.hpp"
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
// footprint() makes a box over `tiles` tiles instead (world x and y, each `tile_size`
// wide on screen, like Cube(tile_size) per tile); it is always one tile tall, a flat slab
// (2x2x1, 5x5x1).
class Cube {
public:
    explicit Cube(float size, ui::Color front = DefaultFront, ui::Color top = DefaultTop)
        : Cube(size, size, size, front, top) {}

    static Cube footprint(float tile_size, ::world::TileSize tiles,
                          ui::Color front = DefaultFront, ui::Color top = DefaultTop) {
        return Cube(tile_size * static_cast<float>(tiles.width),
                    tile_size * static_cast<float>(tiles.height), tile_size, front, top);
    }

    // How tall its vertical edges are on screen.
    float height() const;

    CubeFaces faces(::world::ScreenPoint ground) const;

    // Everything the cube covers when drawn at `ground`.
    ui::Rect bounds(::world::ScreenPoint ground) const;

    void draw(ui::Canvas& canvas, ::world::ScreenPoint ground) const;

private:
    static constexpr ui::Color DefaultFront{200, 160, 50, 255};
    static constexpr ui::Color DefaultTop{240, 210, 110, 255};

    Cube(float along_x, float along_y, float tall, ui::Color front, ui::Color top)
        : along_x_(along_x), along_y_(along_y), tall_(tall), front_(front), top_(top) {}

    // Screen width the base spans along world x and along world y.
    float along_x_;
    float along_y_;
    // The width of the cube whose height it has (height() = tall_ * HeightRatio).
    float tall_;
    ui::Color front_;
    ui::Color top_;
};

} // namespace scene::world
