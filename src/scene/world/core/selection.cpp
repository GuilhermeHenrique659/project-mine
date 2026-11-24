#include "scene/world/core/selection.hpp"

#include <algorithm>
#include <cstdlib>

#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/entities/vehicle.hpp"

namespace world {

bool takes_orders(const core::Registry& registry, core::EntityId id) {
    if (registry.has<Vehicle>(id)) {
        return is_crewed(registry, id) && !carries_player(registry, id);
    }
    return registry.has<Soldier>(id) && !registry.has<Passenger>(id);
}

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
        // Empty vehicles too: they are selected to be boarded or driven later.
        const bool vehicle = registry.has<Vehicle>(entity.id);
        if (position == nullptr || (!vehicle && !registry.has<Soldier>(entity.id)) ||
            registry.has<Passenger>(entity.id)) {
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
