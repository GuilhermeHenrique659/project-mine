#include "scene/world/core/systems/production_system.hpp"

#include <algorithm>
#include <unordered_map>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/worker_report.hpp"

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

// Makes one output when every input is there and the output fits; otherwise nothing
// changes and the reason is returned (None = made one).
IdleReason produce_one(Inventory& inventory, const std::vector<ItemStack>& inputs, Item output) {
    for (const ItemStack& input : inputs) {
        if (inventory.count(input.item) < input.count) {
            return IdleReason::MissingInput;
        }
    }
    for (const ItemStack& input : inputs) {
        inventory.remove(input.item, input.count);
    }
    if (inventory.add(output, 1) == 0) {
        return IdleReason::None;
    }
    // No room: undo the inputs.
    for (const ItemStack& input : inputs) {
        inventory.add(input.item, input.count);
    }
    return IdleReason::OutputFull;
}

// Share of the full rate the workers at a building give: present / needed, at most 1; 1
// for a building without Staffing.
float crew_share(const Staffing* staffing, int present) {
    if (staffing == nullptr || staffing->needed <= 0) {
        return 1.0f;
    }
    return std::min(1.0f, static_cast<float>(present) / static_cast<float>(staffing->needed));
}

// Advances one producer by dt; the reason it is idle, None while it works.
IdleReason run(Producer& producer, Inventory& inventory, const std::optional<Item>& output,
               const PowerConsumer* power, float crew, float delta_time) {
    if (power != nullptr && !power->powered) {
        return IdleReason::NoPower;
    }
    if (!output) {
        return IdleReason::NoResource;
    }
    if (crew <= 0.0f) {
        return IdleReason::NoWorkers;
    }
    producer.progress += producer.recipe.rate * crew * delta_time;
    while (producer.progress >= 1.0f) {
        const IdleReason reason = produce_one(inventory, producer.recipe.inputs, *output);
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
    // Workers standing at their workplace (within BuildReach), per building.
    std::unordered_map<core::EntityId, int> present;
    registry_.each<Worker>([&](core::EntityId id, Worker& worker) {
        if (worker.workplace != 0 && is_at_work(registry_, id)) {
            ++present[worker.workplace];
        }
    });
    registry_.each<Building, Producer, Inventory>(
        [&](core::EntityId id, Building& building, Producer& producer, Inventory& inventory) {
            if (registry_.has<ConstructionSite>(id)) {
                return;
            }
            const std::optional<Item> output =
                producer.recipe.output ? producer.recipe.output
                                       : item_from_tile(terrain_.tile_at(building.tile));
            const IdleReason reason = run(producer, inventory, output,
                                          registry_.get<PowerConsumer>(id),
                                          crew_share(registry_.get<Staffing>(id), present[id]),
                                          delta_time);
            if (BuildingStatus* status = registry_.get<BuildingStatus>(id)) {
                status->state =
                    reason == IdleReason::None ? BuildingState::Working : BuildingState::Idle;
                status->reason = reason;
            }
        });
}

} // namespace world
