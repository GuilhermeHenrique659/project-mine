#include "scene/world/core/targeting.hpp"

#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

core::EntityId shootable_at(const core::Registry& registry, TileCoord tile, core::EntityId exclude) {
    const core::EntityId building = building_at(registry, tile);
    if (building != 0 && building != exclude && registry.has<Health>(building)) {
        return building;
    }
    for (const core::Entity& entity : registry.entities()) {
        const auto* position = registry.get<Position>(entity.id);
        if (entity.id == exclude || position == nullptr || !registry.has<Character>(entity.id) ||
            !registry.has<Health>(entity.id) || registry.has<Passenger>(entity.id) ||
            tile_containing(position->point) != tile) {
            continue;
        }
        return entity.id;
    }
    return 0;
}

} // namespace world
