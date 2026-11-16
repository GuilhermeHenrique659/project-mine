#include "scene/world/core/systems/production_system.hpp"

#include <algorithm>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"

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

// Makes one output when the input is there and the output fits; otherwise nothing changes
// and the reason is returned (None = made one).
IdleReason produce_one(Inventory& inventory, const std::optional<ItemStack>& input, Item output) {
    if (input && inventory.count(input->item) < input->count) {
        return IdleReason::MissingInput;
    }
    if (input) {
        inventory.remove(input->item, input->count);
    }
    if (inventory.add(output, 1) == 0) {
        return IdleReason::None;
    }
    // No room: undo the input.
    if (input) {
        inventory.add(input->item, input->count);
    }
    return IdleReason::OutputFull;
}

// Advances one producer by dt; the reason it is idle, None while it works.
IdleReason run(Producer& producer, Inventory& inventory, const std::optional<Item>& output,
               const PowerConsumer* power, float delta_time) {
    if (power != nullptr && !power->powered) {
        return IdleReason::NoPower;
    }
    if (!output) {
        return IdleReason::NoResource;
    }
    producer.progress += producer.recipe.rate * delta_time;
    while (producer.progress >= 1.0f) {
        const IdleReason reason = produce_one(inventory, producer.recipe.input, *output);
        if (reason != IdleReason::None) {
            // Waiting: keep one output ready, do not bank more.
            producer.progress = 1.0f;
            return reason;
        }
        producer.progress -= 1.0f;
    }
    return IdleReason::None;
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
            const IdleReason reason = run(producer, inventory, output,
                                          registry_.get<PowerConsumer>(id), delta_time);
            if (BuildingStatus* status = registry_.get<BuildingStatus>(id)) {
                status->state =
                    reason == IdleReason::None ? BuildingState::Working : BuildingState::Idle;
                status->reason = reason;
            }
        });
}

} // namespace world
