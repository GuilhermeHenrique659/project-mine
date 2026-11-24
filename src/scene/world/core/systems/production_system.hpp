#pragma once

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/items/item.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// The item an ore tile yields (iron ore tile -> IronOre); empty for any other tile.
std::optional<Item> item_from_tile(TileType type);

// The loop every kind of production shares (abstract: a subclass says which recipes it
// handles and what one craft does). For every finished building (Building + Producer + a
// SolidInventory or LiquidInventory, no ConstructionSite) whose recipe it handles: stops
// while its PowerConsumer is unpowered (NoPower), while there is nothing to make
// (has_output() false: NoResource) or no worker is present (NoWorkers); otherwise its
// progress grows by rate * crew share * dt and each whole unit is one craft(). A refused
// craft (MissingInput, OutputFull) waits with at most one unit of progress banked. A
// building with Staffing runs at rate * min(present, needed) / needed, present = workers
// whose workplace it is and that stand within BuildReach. Its BuildingStatus (when it has
// one) becomes Idle with the reason, or Working. The registry must outlive it.
class ProductionSystem : public System {
public:
    void update(float delta_time) final;

protected:
    explicit ProductionSystem(core::Registry& registry) : registry_(registry) {}

    // Whether this system runs the producers with this recipe.
    virtual bool handles(const Recipe& recipe) const = 0;
    // Whether the building has anything to make at all (a mine off ore does not).
    virtual bool has_output(const Building&, const Recipe&) const { return true; }
    // Makes one craft; None = made, otherwise why not (then nothing changed).
    virtual IdleReason craft(const Building& building, Inventory& inventory,
                             const Recipe& recipe) = 0;

    // Whether every input of the recipe is in the inventory.
    static bool has_inputs(const Inventory& inventory, const Recipe& recipe);

    core::Registry& registry_;

private:
    IdleReason run(core::EntityId id, const Building& building, Producer& producer,
                   Inventory& inventory, int present, float delta_time);
};

} // namespace world
