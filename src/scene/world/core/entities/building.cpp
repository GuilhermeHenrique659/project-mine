#include "scene/world/core/entities/building.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"

namespace world {

core::EntityId spawn_building(core::Registry& registry, std::string label) {
    const core::EntityId id = registry.create(std::move(label));
    registry.add(id, Building{});
    registry.add(id, Position{});
    return id;
}

void make_building(core::Registry& registry, core::EntityId id, const Construction& construction,
                   TileCoord tile) {
    registry.add(id, Building{*construction.type, tile});
    registry.add(id, Position{{tile.x + 0.5f, tile.y + 0.5f}});
    if (construction.effort <= 0.0f) {
        registry.remove<ConstructionSite>(id);
        return;
    }
    registry.add(id, ConstructionSite{0.0f, construction.effort});
}

} // namespace world
