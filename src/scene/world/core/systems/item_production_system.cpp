#include "scene/world/core/systems/item_production_system.hpp"

namespace world {

std::optional<Item> ItemProductionSystem::output_of(const Building& building,
                                                    const Recipe& recipe) const {
    return recipe.output ? recipe.output : item_from_tile(terrain_.tile_at(building.tile));
}

bool ItemProductionSystem::has_output(const Building& building, const Recipe& recipe) const {
    return output_of(building, recipe).has_value();
}

IdleReason ItemProductionSystem::craft(const Building& building, Inventory& inventory,
                                       const Recipe& recipe) {
    if (!has_inputs(inventory, recipe)) {
        return IdleReason::MissingInput;
    }
    const Item output = *output_of(building, recipe);
    for (const ItemStack& input : recipe.inputs) {
        inventory.remove(input.item, input.count);
    }
    const int left = inventory.add(output, recipe.output_count);
    if (left == 0) {
        return IdleReason::None;
    }
    // No room: undo the outputs that fit and the inputs.
    inventory.remove(output, recipe.output_count - left);
    for (const ItemStack& input : recipe.inputs) {
        inventory.add(input.item, input.count);
    }
    return IdleReason::OutputFull;
}

} // namespace world
