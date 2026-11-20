#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/systems/power_system.hpp"

namespace {

struct Fixture {
    core::Registry registry;
    world::PowerSystem system{registry};

    core::EntityId generator(world::GeneratorSpec spec) {
        const core::EntityId id = registry.create("Generator");
        registry.add(id, world::Generator{spec});
        registry.add(id, world::SolidInventory{4});
        return id;
    }

    core::EntityId solar(float power_kw) {
        return generator({world::GeneratorType::Solar, std::nullopt, 0.0f, power_kw});
    }

    core::EntityId consumer(float demand_kw) {
        const core::EntityId id = registry.create("Consumer");
        registry.add(id, world::PowerConsumer{demand_kw});
        return id;
    }

    bool powered(core::EntityId id) const {
        return registry.get<world::PowerConsumer>(id)->powered;
    }
};

} // namespace

TEST(PowerSystem, TheHeadquartersGeneratesSolarPowerAndTheMineConsumesIt) {
    // Given / When
    const auto* hq = world::find_construction(world::BuildingType::Headquarters);
    const auto* mine = world::find_construction(world::BuildingType::Mine);
    const auto* generator = hq->blueprint.find<world::Generator>();
    const auto* consumer = mine->blueprint.find<world::PowerConsumer>();

    // Then
    ASSERT_NE(generator, nullptr);
    EXPECT_EQ(generator->spec.type, world::GeneratorType::Solar);
    EXPECT_FALSE(generator->spec.input.has_value());
    EXPECT_FLOAT_EQ(generator->spec.power_kw, 1000.0f);
    ASSERT_NE(consumer, nullptr);
    EXPECT_FLOAT_EQ(consumer->demand_kw, 100.0f);
}

TEST(PowerSystem, MakingBuildingsGivesThemTheirGeneratorAndConsumer) {
    // Given
    core::Registry registry;
    const core::EntityId hq = registry.create("HQ");
    const core::EntityId mine = registry.create("Mine");

    // When
    world::make_building(registry, hq,
                         *world::find_construction(world::BuildingType::Headquarters), {0, 0});
    world::make_building(registry, mine, *world::find_construction(world::BuildingType::Mine),
                         {1, 1});

    // Then
    ASSERT_NE(registry.get<world::Generator>(hq), nullptr);
    EXPECT_FLOAT_EQ(registry.get<world::Generator>(hq)->spec.power_kw, 1000.0f);
    EXPECT_EQ(registry.get<world::PowerConsumer>(hq), nullptr);
    ASSERT_NE(registry.get<world::PowerConsumer>(mine), nullptr);
    EXPECT_FLOAT_EQ(registry.get<world::PowerConsumer>(mine)->demand_kw, 100.0f);
    EXPECT_EQ(registry.get<world::Generator>(mine), nullptr);
}

TEST(PowerSystem, ConsumersArePoweredWhileDemandDoesNotExceedSupply) {
    // Given: ten mines use up exactly the 1000 kW of the headquarters
    Fixture f;
    f.solar(1000.0f);
    std::vector<core::EntityId> mines;
    for (int i = 0; i < 10; ++i) {
        mines.push_back(f.consumer(100.0f));
    }

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(f.system.supply_kw(), 1000.0f);
    EXPECT_FLOAT_EQ(f.system.demand_kw(), 1000.0f);
    for (const core::EntityId mine : mines) {
        EXPECT_TRUE(f.powered(mine));
    }
}

TEST(PowerSystem, AllConsumersLosePowerWhenDemandExceedsSupply) {
    // Given: eleven mines need 1100 kW
    Fixture f;
    f.solar(1000.0f);
    std::vector<core::EntityId> mines;
    for (int i = 0; i < 11; ++i) {
        mines.push_back(f.consumer(100.0f));
    }

    // When
    f.system.update(1.0f);

    // Then
    for (const core::EntityId mine : mines) {
        EXPECT_FALSE(f.powered(mine));
    }
}

TEST(PowerSystem, ConsumersRegainPowerOnceSupplyCoversDemand) {
    // Given
    Fixture f;
    f.solar(1000.0f);
    const core::EntityId mine = f.consumer(100.0f);
    const core::EntityId extra = f.consumer(1000.0f);
    f.system.update(1.0f);

    // When
    f.registry.destroy(extra);
    f.system.update(1.0f);

    // Then
    EXPECT_TRUE(f.powered(mine));
}

TEST(PowerSystem, ConstructionSitesNeitherGenerateNorConsume) {
    // Given: an unfinished generator and an unfinished hungry consumer
    Fixture f;
    f.solar(1000.0f);
    const core::EntityId site_generator = f.solar(5000.0f);
    f.registry.add(site_generator, world::ConstructionSite{0.0f, 10.0f});
    const core::EntityId site_consumer = f.consumer(5000.0f);
    f.registry.add(site_consumer, world::ConstructionSite{0.0f, 10.0f});
    const core::EntityId mine = f.consumer(100.0f);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(f.system.supply_kw(), 1000.0f);
    EXPECT_FLOAT_EQ(f.system.demand_kw(), 100.0f);
    EXPECT_TRUE(f.powered(mine));
}

namespace {

world::GeneratorSpec coal_spec() {
    return {world::GeneratorType::Coal, world::ItemStack{world::items::Coal, 1}, 1.0f, 1000.0f};
}

// Runs the system for `seconds` in 0.1 s frames, handing the generator one coal every
// `coal_every` seconds (0 = none).
void run(Fixture& f, core::EntityId generator, float seconds, float coal_every) {
    float until_coal = 0.0f;
    for (int frame = 0; frame < static_cast<int>(seconds * 10.0f + 0.5f); ++frame) {
        if (coal_every > 0.0f && until_coal <= 0.0f) {
            f.registry.get<world::SolidInventory>(generator)->add(world::items::Coal, 1);
            until_coal += coal_every;
        }
        until_coal -= 0.1f;
        f.system.update(0.1f);
    }
}

} // namespace

TEST(PowerSystem, AFullyFuelledCoalGeneratorBurnsOneCoalPerSecondFor1000Kw) {
    // Given
    Fixture f;
    const core::EntityId id = f.generator(coal_spec());
    f.registry.get<world::SolidInventory>(id)->add(world::items::Coal, 100);

    // When
    run(f, id, 60.0f, 0.0f);

    // Then
    EXPECT_NEAR(f.system.supply_kw(), 1000.0f, 5.0f); // still finishing its warm-up
    EXPECT_NEAR(f.registry.get<world::SolidInventory>(id)->count(world::items::Coal), 40, 1);
}

TEST(PowerSystem, ACoalGeneratorFedHalfItsCoalRateProducesAboutHalfItsPower) {
    // Given: one coal every 2 s instead of every second
    Fixture f;
    const core::EntityId id = f.generator(coal_spec());

    // When
    run(f, id, 120.0f, 2.0f);

    // Then
    EXPECT_NEAR(f.system.supply_kw(), 500.0f, 100.0f);
}

TEST(PowerSystem, ACoalGeneratorWithoutCoalProducesNothing) {
    // Given
    Fixture f;
    const core::EntityId id = f.generator(coal_spec());
    const core::EntityId mine = f.consumer(100.0f);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(f.system.supply_kw(), 0.0f);
    EXPECT_FALSE(f.registry.get<world::Generator>(id)->running);
    EXPECT_FALSE(f.powered(mine));
}

TEST(PowerSystem, ACoalGeneratorThatRunsOutOfCoalFadesToNothing) {
    // Given: 10 s of coal
    Fixture f;
    const core::EntityId id = f.generator(coal_spec());
    f.registry.get<world::SolidInventory>(id)->add(world::items::Coal, 10);

    // When
    run(f, id, 60.0f, 0.0f);

    // Then
    EXPECT_EQ(f.registry.get<world::SolidInventory>(id)->count(world::items::Coal), 0);
    EXPECT_LT(f.system.supply_kw(), 50.0f);
}
