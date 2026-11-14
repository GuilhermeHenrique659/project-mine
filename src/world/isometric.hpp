#pragma once

#include <array>

// Math of the isometric world (2:1 diamond projection). Raylib-free.
//
// World space is measured in tiles: tile (x, y) covers [x, x+1) x [y, y+1).
// World +x goes right-down on screen and world +y goes left-down, so a tile
// is drawn as a diamond `tile_width` wide and `tile_height` tall.
namespace world {

// Integer grid cell.
struct TileCoord {
    int x = 0;
    int y = 0;

    bool operator==(const TileCoord&) const = default;
};

// Continuous position in world space, in tiles.
struct WorldPoint {
    float x = 0.0f;
    float y = 0.0f;
};

// Position on screen, in pixels.
struct ScreenPoint {
    float x = 0.0f;
    float y = 0.0f;
};

// Corners of a tile's diamond on screen.
struct TileCorners {
    ScreenPoint top;
    ScreenPoint right;
    ScreenPoint bottom;
    ScreenPoint left;
};

// The tile that contains a world position (floors, so -0.5 is in tile -1).
TileCoord tile_containing(WorldPoint point);

// Converts between world space and the screen.
struct IsoProjection {
    float tile_width = 64.0f;
    float tile_height = 32.0f;
    // Screen position of world (0, 0), the top corner of tile (0, 0).
    // Move it to scroll the camera; scale the tile size to zoom.
    ScreenPoint origin{};

    ScreenPoint world_to_screen(WorldPoint point) const;
    WorldPoint screen_to_world(ScreenPoint point) const;

    // Tile under a screen position (mouse picking).
    TileCoord tile_at(ScreenPoint point) const;

    ScreenPoint tile_center(TileCoord tile) const;
    TileCorners tile_corners(TileCoord tile) const;
};

// Converts a movement direction on screen (y grows downwards: up is (0, -1)) to a world
// direction, so that pressing up moves up on screen in the isometric view.
WorldPoint screen_direction_to_world(ScreenPoint direction);

// Painter's order: larger depth is closer to the viewer and is drawn later.
float depth_of(WorldPoint point);
int depth_of(TileCoord tile);

// Strict weak ordering for sorting tiles back to front. Tiles with the same
// depth never overlap; they are ordered left to right to keep it stable.
bool draws_before(TileCoord a, TileCoord b);

} // namespace world
