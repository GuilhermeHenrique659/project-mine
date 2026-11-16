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

namespace {

float centre_depth(const DepthBox& box) {
    return depth_of(WorldPoint{(box.min.x + box.max.x) / 2.0f, (box.min.y + box.max.y) / 2.0f});
}

// Whether `back` must be drawn before `front`: `front` is past it along one axis and not
// behind it along the other (side by side boxes have no order).
bool must_draw_before(const DepthBox& back, const DepthBox& front) {
    const bool past_x = front.min.x >= back.max.x;
    const bool past_y = front.min.y >= back.max.y;
    const bool behind_x = back.min.x >= front.max.x;
    const bool behind_y = back.min.y >= front.max.y;
    return (past_x && !behind_y) || (past_y && !behind_x);
}

} // namespace

std::vector<std::size_t> painter_order(const std::vector<DepthBox>& boxes) {
    const std::size_t count = boxes.size();
    // blockers[i] = how many boxes must still be drawn before box i.
    std::vector<int> blockers(count, 0);
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = 0; j < count; ++j) {
            if (i != j && must_draw_before(boxes[j], boxes[i])) ++blockers[i];
        }
    }

    std::vector<std::size_t> order;
    order.reserve(count);
    std::vector<bool> drawn(count, false);
    while (order.size() < count) {
        // The free box with the lowest depth; on a cycle (should not happen with
        // non-overlapping boxes) the lowest depth overall, so drawing always goes on.
        std::size_t next = count;
        bool next_free = false;
        for (std::size_t i = 0; i < count; ++i) {
            if (drawn[i]) continue;
            const bool free = blockers[i] == 0;
            if (next == count || (free && !next_free) ||
                (free == next_free && centre_depth(boxes[i]) < centre_depth(boxes[next]))) {
                next = i;
                next_free = free;
            }
        }
        drawn[next] = true;
        order.push_back(next);
        for (std::size_t i = 0; i < count; ++i) {
            if (!drawn[i] && must_draw_before(boxes[next], boxes[i])) --blockers[i];
        }
    }
    return order;
}

} // namespace world
