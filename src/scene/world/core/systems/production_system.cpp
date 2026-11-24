#include "scene/world/core/systems/production_system.hpp"

#include <algorithm>
#include <unordered_map>

#include "scene/world/core/components/liquid_inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/worker_report.hpp"

namespace world {

namespace {
// Slack on "one output is done": slow rates (1/60 per second) add up to just under 1 in
// float, which would delay the output by a whole tick.
constexpr float ProgressEpsilon = 1e-4f;

// Share of the full rate the workers at a building give: present / needed, at most 1; 1
// for a building without Staffing.
float crew_share(const Staffing* staffing, int present) {
    if (staffing == nullptr || staffing->needed <= 0) {
        return 1.0f;
    }
    return std::min(1.0f, static_cast<float>(present) / static_cast<float>(staffing->needed));
}
} // namespace

std::optional<Item> item_from_tile(TileType type) {
    switch (type) {
        case TileType::IronOre: return items::IronOre;
        case TileType::CopperOre: return items::CopperOre;
        case TileType::Coal: return items::Coal;
        case TileType::CrudeOil: return items::Oil;
        case TileType::Grass:
        case TileType::Water: break;
    }
    return std::nullopt;
}

bool ProductionSystem::has_inputs(const Inventory& inventory, const Recipe& recipe) {
    return std::all_of(recipe.inputs.begin(), recipe.inputs.end(), [&](const ItemStack& input) {
        return inventory.count(input.item) >= input.count;
    });
}

void ProductionSystem::update(float delta_time) {
    // Workers standing at their workplace (within BuildReach), per building.
    std::unordered_map<core::EntityId, int> present;
    registry_.each<Worker>([&](core::EntityId id, Worker& worker) {
        if (worker.workplace != 0 && is_at_work(registry_, id)) {
            ++present[worker.workplace];
        }
    });
    registry_.each<Building, Producer>(
        [&](core::EntityId id, Building& building, Producer& producer) {
            if (registry_.has<ConstructionSite>(id) || !handles(producer.recipe)) {
                return;
            }
            Inventory* inventory = registry_.get<SolidInventory>(id);
            if (inventory == nullptr) {
                inventory = registry_.get<LiquidInventory>(id);
            }
            if (inventory == nullptr) {
                return;
            }
            const IdleReason reason =
                run(id, building, producer, *inventory, present[id], delta_time);
            if (BuildingStatus* status = registry_.get<BuildingStatus>(id)) {
                status->state =
                    reason == IdleReason::None ? BuildingState::Working : BuildingState::Idle;
                status->reason = reason;
            }
        });
}

IdleReason ProductionSystem::run(core::EntityId id, const Building& building, Producer& producer,
                                 Inventory& inventory, int present, float delta_time) {
    const PowerConsumer* power = registry_.get<PowerConsumer>(id);
    if (power != nullptr && !power->powered) {
        return IdleReason::NoPower;
    }
    if (!has_output(building, producer.recipe)) {
        return IdleReason::NoResource;
    }
    const float crew = crew_share(registry_.get<Staffing>(id), present);
    if (crew <= 0.0f) {
        return IdleReason::NoWorkers;
    }
    producer.progress += producer.recipe.rate * crew * delta_time;
    while (producer.progress >= 1.0f - ProgressEpsilon) {
        const IdleReason reason = craft(building, inventory, producer.recipe);
        if (reason != IdleReason::None) {
            // Waiting: keep one output ready, do not bank more.
            producer.progress = 1.0f;
            return reason;
        }
        producer.progress = std::max(0.0f, producer.progress - 1.0f);
    }
    return IdleReason::None;
}

} // namespace world
