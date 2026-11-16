#pragma once

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/items/item.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// The item an ore tile yields (iron ore tile -> IronOre); empty for any other tile.
std::optional<Item> item_from_tile(TileType type);

// Runs the Producer of every finished building (Building + Producer + Inventory, no
// ConstructionSite): its progress grows by rate * dt and each whole unit consumes the
// recipe's input from the building's inventory and adds one output to it. The output of a
// recipe without one is the ore under the building. Production waits (keeping at most one
// unit of progress) while the input is missing, the output does not fit or there is no
// output (a mine off ore), and stops while its PowerConsumer is unpowered. Its
// BuildingStatus (when it has one) becomes Idle with the reason, or Working. The registry and terrain must outlive it.
class ProductionSystem : public System {
public:
    ProductionSystem(core::Registry& registry, const Terrain& terrain);

    void update(float delta_time) override;

private:
    core::Registry& registry_;
    const Terrain& terrain_;
};

} // namespace world
