#include "scene/world/core/entities/building.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

core::EntityId spawn_building(core::Registry& registry, std::string label) {
    track_buildings(registry);
    const core::EntityId id = registry.create(std::move(label));
    registry.add(id, Building{});
    registry.add(id, Position{});
    return id;
}

void make_building(core::Registry& registry, core::EntityId id, const Construction& construction,
                   TileCoord tile) {
    track_buildings(registry);
    registry.add(id, Building{*construction.type, tile, construction.size});
    registry.add(id, Position{footprint_centre(tile, construction.size)});
    construction.blueprint.apply(registry, id);
    if (construction.effort <= 0.0f) {
        registry.remove<ConstructionSite>(id);
        return;
    }
    registry.add(id, ConstructionSite{0.0f, construction.effort});
}

} // namespace world
