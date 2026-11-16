#include "scene/world/core/systems/power_system.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"

namespace world {

namespace {

// Advances a fuelled generator by dt; true while it has the fuel to run.
bool burn_fuel(Generator& generator, Inventory* inventory, float delta_time) {
    const ItemStack& input = *generator.spec.input;
    generator.progress += generator.spec.rate * delta_time;
    while (generator.progress >= 1.0f) {
        if (inventory == nullptr || inventory->count(input.item) < input.count) {
            // Out of fuel: keep one burn pending, do not bank more.
            generator.progress = 1.0f;
            return false;
        }
        inventory->remove(input.item, input.count);
        generator.progress -= 1.0f;
    }
    return true;
}

} // namespace

PowerSystem::PowerSystem(core::Registry& registry) : registry_(registry) {}

void PowerSystem::update(float delta_time) {
    supply_kw_ = 0.0f;
    demand_kw_ = 0.0f;
    registry_.each<Generator>([&](core::EntityId id, Generator& generator) {
        if (registry_.has<ConstructionSite>(id)) {
            return;
        }
        if (generator.spec.input &&
            !burn_fuel(generator, registry_.get<Inventory>(id), delta_time)) {
            return;
        }
        supply_kw_ += generator.spec.power_kw;
    });
    registry_.each<PowerConsumer>([&](core::EntityId id, PowerConsumer& consumer) {
        if (!registry_.has<ConstructionSite>(id)) {
            demand_kw_ += consumer.demand_kw;
        }
    });
    const bool powered = demand_kw_ <= supply_kw_;
    registry_.each<PowerConsumer>(
        [&](core::EntityId, PowerConsumer& consumer) { consumer.powered = powered; });
}

} // namespace world
