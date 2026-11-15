#include "scene/world/core/systems/production_system.hpp"

#include <algorithm>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"

namespace world {

std::optional<Item> item_from_tile(TileType type) {
    switch (type) {
        case TileType::IronOre: return items::IronOre;
        case TileType::CopperOre: return items::CopperOre;
        case TileType::Coal: return items::Coal;
        case TileType::Grass:
        case TileType::Water: break;
    }
    return std::nullopt;
}

namespace {

// Makes one output when the input is there and the output fits; false otherwise (nothing
// changes then).
bool produce_one(Inventory& inventory, const std::optional<ItemStack>& input, Item output) {
    if (input && inventory.count(input->item) < input->count) {
        return false;
    }
    if (input) {
        inventory.remove(input->item, input->count);
    }
    if (inventory.add(output, 1) == 0) {
        return true;
    }
    // No room: undo the input.
    if (input) {
        inventory.add(input->item, input->count);
    }
    return false;
}

} // namespace

ProductionSystem::ProductionSystem(core::Registry& registry, const Terrain& terrain)
    : registry_(registry), terrain_(terrain) {}

void ProductionSystem::update(float delta_time) {
    registry_.each<Building, Producer, Inventory>(
        [&](core::EntityId id, Building& building, Producer& producer, Inventory& inventory) {
            if (registry_.has<ConstructionSite>(id)) {
                return;
            }
            const std::optional<Item> output =
                producer.recipe.output ? producer.recipe.output
                                       : item_from_tile(terrain_.tile_at(building.tile));
            if (!output) {
                return;
            }
            producer.progress += producer.recipe.rate * delta_time;
            while (producer.progress >= 1.0f) {
                if (!produce_one(inventory, producer.recipe.input, *output)) {
                    // Waiting: keep one output ready, do not bank more.
                    producer.progress = 1.0f;
                    return;
                }
                producer.progress -= 1.0f;
            }
        });
}

} // namespace world
