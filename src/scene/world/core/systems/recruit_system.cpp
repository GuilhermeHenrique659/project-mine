#include "scene/world/core/systems/recruit_system.hpp"

#include <algorithm>
#include <optional>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/events/military_events.hpp"
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

RecruitSystem::RecruitSystem(core::Registry& registry, event::EventBus& bus,
                             const EntityFactory& factory, const Terrain& terrain)
    : registry_(registry),
      factory_(factory),
      terrain_(terrain),
      subscription_(event::subscribe_scoped<SoldierRecruitRequested>(
          bus, [this](const SoldierRecruitRequested& request) { recruit(request.building); })) {}

std::optional<TileCoord> RecruitSystem::spawn_tile(const Building& building) const {
    const auto is_free = [this](TileCoord tile) {
        return building_at(registry_, tile) == 0 && terrain_.tile_at(tile) != TileType::Water &&
               !unit_on(registry_, tile);
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

void RecruitSystem::recruit(core::EntityId building) {
    const auto* at = registry_.get<Position>(building);
    const auto* footprint = registry_.get<Building>(building);
    if (at == nullptr || footprint == nullptr || !registry_.has<Recruiter>(building) ||
        registry_.has<ConstructionSite>(building)) {
        return;
    }
    const auto* power = registry_.get<PowerConsumer>(building);
    if (power != nullptr && !power->powered) {
        return;
    }
    // Out of the barracks on a free tile; on the building itself only when none is free.
    WorldPoint point = at->point;
    if (const auto tile = spawn_tile(*footprint)) {
        point = {tile->x + 0.5f, tile->y + 0.5f};
    }
    const auto id = factory_.spawn(registry_, SoldierArchetype, "Soldier");
    if (!id) {
        return;
    }
    registry_.get<Position>(*id)->point = point;
}

} // namespace world
