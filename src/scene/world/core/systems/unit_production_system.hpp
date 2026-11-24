#pragma once

#include "core/registry.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/systems/production_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Production of entities (see ProductionSystem for the shared loop): the recipes with a
// UnitOutput, like the Tank Factory's Battle Tank. Each craft consumes every input stack
// and spawns the archetype through `factory` on the nearest free tile around the building
// (spawn_tile(), like a recruit); with no free tile it waits (OutputFull) and keeps the
// inputs. Everything passed must outlive it.
class UnitProductionSystem : public ProductionSystem {
public:
    UnitProductionSystem(core::Registry& registry, const Terrain& terrain,
                         const EntityFactory& factory)
        : ProductionSystem(registry), terrain_(terrain), factory_(factory) {}

protected:
    bool handles(const Recipe& recipe) const override { return recipe.spawns.has_value(); }
    IdleReason craft(const Building& building, Inventory& inventory, const Recipe& recipe) override;

private:
    const Terrain& terrain_;
    const EntityFactory& factory_;
};

} // namespace world
