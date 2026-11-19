#include "scene/world/core/spawn.hpp"

#include <algorithm>

#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

namespace {

// Whether a unit already stands on `tile`.
bool unit_on(const core::Registry& registry, TileCoord tile) {
    const auto& entities = registry.entities();
    return std::any_of(entities.begin(), entities.end(), [&](const core::Entity& entity) {
        const Position* position = registry.get<Position>(entity.id);
        return position != nullptr && registry.has<Unit>(entity.id) &&
               tile_containing(position->point) == tile;
    });
}

} // namespace

std::optional<TileCoord> spawn_tile(const core::Registry& registry, const Terrain& terrain,
                                    const Building& building) {
    const auto is_free = [&](TileCoord tile) {
        return building_at(registry, tile) == 0 && terrain.tile_at(tile) != TileType::Water &&
               !unit_on(registry, tile);
    };
    // Rings around the footprint, nearest first; each ring only adds its border tiles.
    const int left = building.tile.x;
    const int top = building.tile.y;
    const int right = left + building.size.width - 1;
    const int bottom = top + building.size.height - 1;
    for (int ring = 1; ring <= MaxSpawnRing; ++ring) {
        for (int y = top - ring; y <= bottom + ring; ++y) {
            for (int x = left - ring; x <= right + ring; ++x) {
                const bool border = x == left - ring || x == right + ring || y == top - ring ||
                                    y == bottom + ring;
                if (border && is_free({x, y})) {
                    return TileCoord{x, y};
                }
            }
        }
    }
    return std::nullopt;
}

} // namespace world
