#include "scene/world/core/occupancy.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "scene/world/core/components/position.hpp"

namespace world {

namespace {

bool covers(const Building& building, TileCoord tile) {
    return tile.x >= building.tile.x && tile.x < building.tile.x + building.size.width &&
           tile.y >= building.tile.y && tile.y < building.tile.y + building.size.height;
}

// Marks a registry whose Occupancy hooks are installed (the Occupancy itself may be
// created by anyone through context<>()).
struct OccupancyTracked {
    bool installed = false;
};

} // namespace

std::vector<TileCoord> footprint_tiles(const Building& building) {
    std::vector<TileCoord> tiles;
    for (int dy = 0; dy < building.size.height; ++dy) {
        for (int dx = 0; dx < building.size.width; ++dx) {
            tiles.push_back({building.tile.x + dx, building.tile.y + dy});
        }
    }
    return tiles;
}

WorldPoint footprint_centre(TileCoord origin, TileSize size) {
    return {static_cast<float>(origin.x) + static_cast<float>(size.width) / 2.0f,
            static_cast<float>(origin.y) + static_cast<float>(size.height) / 2.0f};
}

std::uint64_t Occupancy::key_of(TileCoord tile) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(tile.x)) << 32) |
           static_cast<std::uint32_t>(tile.y);
}

void Occupancy::insert(core::EntityId id, const Building& building) {
    for (const TileCoord tile : footprint_tiles(building)) {
        tiles_[key_of(tile)].push_back(id);
    }
}

void Occupancy::erase(core::EntityId id, const Building& building) {
    for (const TileCoord tile : footprint_tiles(building)) {
        const auto found = tiles_.find(key_of(tile));
        if (found == tiles_.end()) {
            continue;
        }
        auto& owners = found->second;
        owners.erase(std::remove(owners.begin(), owners.end(), id), owners.end());
        if (owners.empty()) {
            tiles_.erase(found);
        }
    }
}

core::EntityId Occupancy::at(TileCoord tile) const {
    const auto found = tiles_.find(key_of(tile));
    return found == tiles_.end() ? 0 : found->second.back();
}

void track_buildings(core::Registry& registry) {
    OccupancyTracked& tracked = registry.context<OccupancyTracked>();
    if (tracked.installed) {
        return;
    }
    tracked.installed = true;
    Occupancy& occupancy = registry.context<Occupancy>();
    registry.each<Building>(
        [&occupancy](core::EntityId id, Building& building) { occupancy.insert(id, building); });
    registry.on_add<Building>([&occupancy](core::EntityId id, const Building& building) {
        occupancy.insert(id, building);
    });
    registry.on_remove<Building>([&occupancy](core::EntityId id, const Building& building) {
        occupancy.erase(id, building);
    });
}

core::EntityId building_at(const core::Registry& registry, TileCoord tile) {
    const OccupancyTracked* tracked = registry.find_context<OccupancyTracked>();
    if (tracked != nullptr && tracked->installed) {
        return registry.find_context<Occupancy>()->at(tile);
    }
    for (const core::Entity& entity : registry.entities()) {
        const Building* building = registry.get<Building>(entity.id);
        if (building != nullptr && covers(*building, tile)) {
            return entity.id;
        }
    }
    return 0;
}

float distance_to_building(const core::Registry& registry, core::EntityId id, WorldPoint point) {
    if (const Building* building = registry.get<Building>(id)) {
        const float left = static_cast<float>(building->tile.x);
        const float top = static_cast<float>(building->tile.y);
        const float right = left + static_cast<float>(building->size.width);
        const float bottom = top + static_cast<float>(building->size.height);
        const float dx = std::max({left - point.x, 0.0f, point.x - right});
        const float dy = std::max({top - point.y, 0.0f, point.y - bottom});
        return std::hypot(dx, dy);
    }
    const Position* position = registry.get<Position>(id);
    if (position == nullptr) {
        return std::numeric_limits<float>::infinity();
    }
    return std::hypot(position->point.x - point.x, position->point.y - point.y);
}

} // namespace world
