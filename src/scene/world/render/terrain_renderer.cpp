#include "scene/world/render/terrain_renderer.hpp"

#include <algorithm>
#include <cmath>

#include "scene/world/core/isometric.hpp"

namespace scene::world {

namespace {

ui::Point to_point(::world::ScreenPoint point) { return {point.x, point.y}; }

ui::Color color_of(::world::TileType type) {
    return type == ::world::TileType::Water ? WaterColor : GrassColor;
}

} // namespace

void TerrainRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    const ::world::IsoProjection projection = camera_.projection(screen.width, screen.height);

    // The tiles that can be on screen: the world-space bounds of the four screen corners,
    // plus one tile of margin because a diamond sticks out of its tile's centre.
    const ::world::WorldPoint corners[] = {
        projection.screen_to_world({0.0f, 0.0f}), projection.screen_to_world({screen.width, 0.0f}),
        projection.screen_to_world({0.0f, screen.height}),
        projection.screen_to_world({screen.width, screen.height})};
    float min_x = corners[0].x, max_x = corners[0].x, min_y = corners[0].y, max_y = corners[0].y;
    for (const ::world::WorldPoint& corner : corners) {
        min_x = std::min(min_x, corner.x);
        max_x = std::max(max_x, corner.x);
        min_y = std::min(min_y, corner.y);
        max_y = std::max(max_y, corner.y);
    }
    const int first_x = static_cast<int>(std::floor(min_x)) - 1;
    const int last_x = static_cast<int>(std::floor(max_x)) + 1;
    const int first_y = static_cast<int>(std::floor(min_y)) - 1;
    const int last_y = static_cast<int>(std::floor(max_y)) + 1;

    for (int y = first_y; y <= last_y; ++y) {
        for (int x = first_x; x <= last_x; ++x) {
            const ::world::TileCoord tile{x, y};
            const ::world::TileCorners diamond = projection.tile_corners(tile);
            const ui::Color color = color_of(terrain_.tile_at(tile));
            canvas.fill_triangle(to_point(diamond.top), to_point(diamond.left),
                                 to_point(diamond.bottom), color);
            canvas.fill_triangle(to_point(diamond.top), to_point(diamond.bottom),
                                 to_point(diamond.right), color);
        }
    }
}

} // namespace scene::world
