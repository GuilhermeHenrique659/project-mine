#pragma once

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/systems/production_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Production into the building's inventory (see ProductionSystem for the shared loop): the
// recipes without a UnitOutput. Each craft consumes every input stack and adds
// output_count of the output, all or nothing (MissingInput / OutputFull). A recipe without
// an output makes the ore under the building (NoResource off ore). The registry and
// terrain must outlive it.
class ItemProductionSystem : public ProductionSystem {
public:
    ItemProductionSystem(core::Registry& registry, const Terrain& terrain)
        : ProductionSystem(registry), terrain_(terrain) {}

protected:
    bool handles(const Recipe& recipe) const override { return !recipe.spawns; }
    bool has_output(const Building& building, const Recipe& recipe) const override;
    IdleReason craft(const Building& building, Inventory& inventory, const Recipe& recipe) override;

private:
    std::optional<Item> output_of(const Building& building, const Recipe& recipe) const;

    const Terrain& terrain_;
};

} // namespace world
