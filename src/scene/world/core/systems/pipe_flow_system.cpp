#include "scene/world/core/systems/pipe_flow_system.hpp"

#include <cmath>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/liquid_inventory.hpp"
#include "scene/world/core/events/building_events.hpp"

namespace world {

namespace {

// Moves one unit from one of the producers to one of the consumers, taking turns: starts
// with the pair after the last one that moved (`turn`). False when no pair can move one.
bool move_one(core::Registry& registry, const std::vector<core::EntityId>& producers,
              const std::vector<core::EntityId>& consumers, std::size_t& turn) {
    const std::size_t pairs = producers.size() * consumers.size();
    for (std::size_t tried = 0; tried < pairs; ++tried) {
        const std::size_t pair = (turn + tried) % pairs;
        LiquidInventory* from = registry.get<LiquidInventory>(producers[pair % producers.size()]);
        LiquidInventory* to = registry.get<LiquidInventory>(consumers[pair / producers.size()]);
        if (from == nullptr || to == nullptr || !from->liquid()) {
            continue;
        }
        if (transfer_items(*from, *to, item_of(*from->liquid()), 1) == 1) {
            turn = pair + 1;
            return true;
        }
    }
    return false;
}

} // namespace

PipeFlowSystem::PipeFlowSystem(core::Registry& registry, event::EventBus& bus)
    : registry_(registry),
      placed_(event::subscribe_scoped<BuildingPlaced>(bus, [this](const BuildingPlaced&) { dirty_ = true; })),
      completed_(event::subscribe_scoped<BuildingCompleted>(
          bus, [this](const BuildingCompleted&) { dirty_ = true; })) {}

void PipeFlowSystem::update(float delta_time) {
    if (dirty_) {
        dirty_ = false;
        network_.rebuild(registry_);
        budgets_.assign(network_.groups().size(), 0.0f);
        turns_.assign(network_.groups().size(), 0);
    }
    for (std::size_t index = 0; index < network_.groups().size(); ++index) {
        const PipeGroup& group = network_.groups()[index];
        std::vector<core::EntityId> producers;
        std::vector<core::EntityId> consumers;
        for (const core::EntityId member : group.members) {
            (registry_.has<Producer>(member) ? producers : consumers).push_back(member);
        }
        if (producers.empty() || consumers.empty()) {
            continue;
        }
        float& budget = budgets_[index];
        budget += PipeThroughput * delta_time;
        const int units = static_cast<int>(std::floor(budget));
        budget -= static_cast<float>(units);
        for (int unit = 0; unit < units; ++unit) {
            if (!move_one(registry_, producers, consumers, turns_[index])) {
                // Nothing can move: do not bank the flow for later.
                budget = 0.0f;
                break;
            }
        }
    }
}

} // namespace world
