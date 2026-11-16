#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/power_report.hpp"
#include "scene/world/core/systems/power_system.hpp"

namespace {

struct Network {
    core::Registry registry;
    world::PowerSystem system{registry};

    core::EntityId generator(std::string name, world::GeneratorSpec spec) {
        const core::EntityId id = registry.create(std::move(name));
        registry.add(id, world::Generator{spec});
        registry.add(id, world::Inventory{4});
        return id;
    }

    core::EntityId solar(std::string name, float kw) {
        return generator(std::move(name), {world::GeneratorType::Solar, std::nullopt, 0.0f, kw});
    }

    core::EntityId consumer(std::string name, float kw) {
        const core::EntityId id = registry.create(std::move(name));
        registry.add(id, world::PowerConsumer{kw});
        return id;
    }
};

} // namespace

TEST(PowerReport, ListsGeneratorsAndConsumersWithTheTotals) {
    // Given
    Network n;
    n.solar("Headquarters", 1000.0f);
    n.consumer("Mine", 100.0f);
    n.consumer("Mine", 100.0f);
    n.system.update(1.0f);

    // When
    const world::PowerReport report = world::power_report(n.registry);

    // Then
    ASSERT_EQ(report.generators.size(), 1u);
    EXPECT_EQ(report.generators[0].name, "Headquarters");
    EXPECT_FLOAT_EQ(report.generators[0].kw, 1000.0f);
    EXPECT_TRUE(report.generators[0].active);
    ASSERT_EQ(report.consumers.size(), 2u);
    EXPECT_EQ(report.consumers[0].name, "Mine");
    EXPECT_FLOAT_EQ(report.consumers[0].kw, 100.0f);
    EXPECT_TRUE(report.consumers[0].active);
    EXPECT_FLOAT_EQ(report.produced_kw, 1000.0f);
    EXPECT_FLOAT_EQ(report.demand_kw, 200.0f);
    EXPECT_FALSE(report.overloaded());
}

TEST(PowerReport, DemandAboveProductionIsOverloadedWithEveryConsumerUnpowered) {
    // Given
    Network n;
    n.solar("Headquarters", 1000.0f);
    n.consumer("Big", 1200.0f);
    n.system.update(1.0f);

    // When
    const world::PowerReport report = world::power_report(n.registry);

    // Then
    EXPECT_TRUE(report.overloaded());
    ASSERT_EQ(report.consumers.size(), 1u);
    EXPECT_FALSE(report.consumers[0].active);
}

TEST(PowerReport, ConstructionSitesAreLeftOut) {
    // Given
    Network n;
    const core::EntityId generator = n.solar("Site", 1000.0f);
    n.registry.add(generator, world::ConstructionSite{0.0f, 10.0f});
    const core::EntityId consumer = n.consumer("Site", 100.0f);
    n.registry.add(consumer, world::ConstructionSite{0.0f, 10.0f});
    n.system.update(1.0f);

    // When
    const world::PowerReport report = world::power_report(n.registry);

    // Then
    EXPECT_TRUE(report.generators.empty());
    EXPECT_TRUE(report.consumers.empty());
    EXPECT_FLOAT_EQ(report.produced_kw, 0.0f);
}

TEST(PowerReport, AGeneratorOutOfFuelIsListedAsStoppedAndProducesNothing) {
    // Given: a coal generator with no coal
    Network n;
    n.generator("Coal Generator",
                {world::GeneratorType::Solar, world::ItemStack{world::items::Coal, 1}, 1.0f,
                 500.0f});
    n.system.update(1.0f);

    // When
    const world::PowerReport report = world::power_report(n.registry);

    // Then
    ASSERT_EQ(report.generators.size(), 1u);
    EXPECT_FALSE(report.generators[0].active);
    EXPECT_FLOAT_EQ(report.produced_kw, 0.0f);
}
