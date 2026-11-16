#include "scene/world/core/systems/power_system.hpp"

#include <algorithm>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"

namespace world {

namespace {

// Burns what a fuelled generator needs for dt and returns the share of it that was there.
float burn_fuel(Generator& generator, Inventory* inventory, float delta_time) {
    const ItemStack& input = *generator.spec.input;
    const float needed = generator.spec.rate * delta_time;
    if (needed <= 0.0f) {
        return 0.0f;
    }
    while (generator.fuel < needed && inventory != nullptr &&
           inventory->count(input.item) >= input.count) {
        inventory->remove(input.item, input.count);
        generator.fuel += 1.0f;
    }
    const float used = std::min(generator.fuel, needed);
    generator.fuel -= used;
    return used / needed;
}

void run_generator(Generator& generator, Inventory* inventory, float delta_time) {
    if (!generator.spec.input) {
        generator.efficiency = 1.0f;
    } else {
        const float supplied = burn_fuel(generator, inventory, delta_time);
        const float blend = std::min(1.0f, delta_time / FuelSmoothingTime);
        generator.efficiency += (supplied - generator.efficiency) * blend;
        if (generator.efficiency < 0.001f) {
            generator.efficiency = 0.0f;
        }
    }
    generator.output_kw = generator.spec.power_kw * generator.efficiency;
    generator.running = generator.output_kw > 0.0f;
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
        run_generator(generator, registry_.get<Inventory>(id), delta_time);
        supply_kw_ += generator.output_kw;
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
