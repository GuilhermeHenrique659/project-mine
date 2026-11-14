#include "scene/world/core/isometric.hpp"

#include <cmath>

namespace world {

TileCoord tile_containing(WorldPoint point) {
    return {static_cast<int>(std::floor(point.x)), static_cast<int>(std::floor(point.y))};
}

ScreenPoint IsoProjection::world_to_screen(WorldPoint point) const {
    return {origin.x + (point.x - point.y) * tile_width * 0.5f,
            origin.y + (point.x + point.y) * tile_height * 0.5f};
}

WorldPoint IsoProjection::screen_to_world(ScreenPoint point) const {
    // In half-tile units: across = x - y, down = x + y.
    const float across = (point.x - origin.x) / (tile_width * 0.5f);
    const float down = (point.y - origin.y) / (tile_height * 0.5f);
    return {(down + across) * 0.5f, (down - across) * 0.5f};
}

TileCoord IsoProjection::tile_at(ScreenPoint point) const {
    return tile_containing(screen_to_world(point));
}

ScreenPoint IsoProjection::tile_center(TileCoord tile) const {
    return world_to_screen({static_cast<float>(tile.x) + 0.5f, static_cast<float>(tile.y) + 0.5f});
}

TileCorners IsoProjection::tile_corners(TileCoord tile) const {
    const float x = static_cast<float>(tile.x);
    const float y = static_cast<float>(tile.y);
    return {
        .top = world_to_screen({x, y}),
        .right = world_to_screen({x + 1.0f, y}),
        .bottom = world_to_screen({x + 1.0f, y + 1.0f}),
        .left = world_to_screen({x, y + 1.0f}),
    };
}

WorldPoint screen_direction_to_world(ScreenPoint direction) {
    // Screen right is world (+1, -1) and screen down is world (+1, +1).
    return {direction.x + direction.y, direction.y - direction.x};
}

float depth_of(WorldPoint point) { return point.x + point.y; }

int depth_of(TileCoord tile) { return tile.x + tile.y; }

bool draws_before(TileCoord a, TileCoord b) {
    const int depth_a = depth_of(a);
    const int depth_b = depth_of(b);
    if (depth_a != depth_b) return depth_a < depth_b;
    return a.x < b.x;
}

} // namespace world
