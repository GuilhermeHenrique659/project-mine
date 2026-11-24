#include "scene/world/core/systems/unit_production_system.hpp"

#include "scene/world/core/components/position.hpp"
#include "scene/world/core/spawn.hpp"

namespace world {

IdleReason UnitProductionSystem::craft(const Building& building, Inventory& inventory,
                                       const Recipe& recipe) {
    if (!has_inputs(inventory, recipe)) {
        return IdleReason::MissingInput;
    }
    // Rolled out beside the building, like a recruit; nowhere free = it waits.
    const auto tile = spawn_tile(registry_, terrain_, building);
    if (!tile) {
        return IdleReason::OutputFull;
    }
    const auto made = factory_.spawn(registry_, recipe.spawns->archetype, recipe.spawns->label);
    if (!made) {
        return IdleReason::OutputFull;
    }
    if (Position* position = registry_.get<Position>(*made)) {
        position->point = {tile->x + 0.5f, tile->y + 0.5f};
    }
    for (const ItemStack& input : recipe.inputs) {
        inventory.remove(input.item, input.count);
    }
    return IdleReason::None;
}

} // namespace world
