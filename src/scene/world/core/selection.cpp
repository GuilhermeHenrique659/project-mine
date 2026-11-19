#include "scene/world/core/selection.hpp"

#include <algorithm>
#include <cstdlib>

#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/position.hpp"

namespace world {

std::vector<core::EntityId> soldiers_in(const core::Registry& registry,
                                        const IsoProjection& projection, ScreenPoint a,
                                        ScreenPoint b) {
    const float left = std::min(a.x, b.x);
    const float right = std::max(a.x, b.x);
    const float top = std::min(a.y, b.y);
    const float bottom = std::max(a.y, b.y);
    std::vector<core::EntityId> selected;
    for (const core::Entity& entity : registry.entities()) {
        const Position* position = registry.get<Position>(entity.id);
        if (position == nullptr || !registry.has<Soldier>(entity.id)) {
            continue;
        }
        const ScreenPoint at = projection.world_to_screen(position->point);
        if (at.x >= left && at.x <= right && at.y >= top && at.y <= bottom) {
            selected.push_back(entity.id);
        }
    }
    return selected;
}

std::vector<TileCoord> formation_tiles(TileCoord goal, std::size_t count,
                                       const BlockedFn& is_blocked) {
    std::vector<TileCoord> tiles;
    // Rings of growing radius around the goal; each ring only adds its border tiles.
    for (int radius = 0; radius <= MaxFormationRadius && tiles.size() < count; ++radius) {
        for (int dy = -radius; dy <= radius && tiles.size() < count; ++dy) {
            for (int dx = -radius; dx <= radius && tiles.size() < count; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                    continue;
                }
                const TileCoord tile{goal.x + dx, goal.y + dy};
                if (!is_blocked(tile)) {
                    tiles.push_back(tile);
                }
            }
        }
    }
    return tiles;
}

} // namespace world
