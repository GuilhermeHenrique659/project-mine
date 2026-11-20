#include <gtest/gtest.h>

#include <algorithm>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/liquid_inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/core/pipes/pipe_network.hpp"
#include "scene/world/core/systems/pipe_flow_system.hpp"

namespace {

using world::BuildingType;
using world::TileCoord;

struct Fixture {
    core::Registry registry;
    event::EventBus bus;

    // A finished catalog building on `tile`.
    core::EntityId add(BuildingType type, TileCoord tile) {
        const core::EntityId id = site(type, tile);
        registry.remove<world::ConstructionSite>(id);
        return id;
    }

    // A catalog building still under construction on `tile`.
    core::EntityId site(BuildingType type, TileCoord tile) {
        const core::EntityId id = registry.create("building");
        world::make_building(registry, id, *world::find_construction(type), tile);
        return id;
    }

    world::LiquidInventory& tank(core::EntityId id) {
        return *registry.get<world::LiquidInventory>(id);
    }
};

bool contains(const std::vector<core::EntityId>& ids, core::EntityId id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // namespace

TEST(ConstructionCatalog, PipeIsAOneTileBuildingCosting10SteelAnd100WorkWithoutPower) {
    // Given / When
    const world::Construction* pipe = world::find_construction(BuildingType::Pipe);

    // Then
    ASSERT_NE(pipe, nullptr);
    EXPECT_EQ(pipe->size, (world::TileSize{1, 1}));
    ASSERT_EQ(pipe->cost.size(), 1u);
    EXPECT_EQ(pipe->cost[0].item, world::items::Steel);
    EXPECT_EQ(pipe->cost[0].count, 10);
    EXPECT_FLOAT_EQ(pipe->effort, 100.0f);
    EXPECT_EQ(pipe->blueprint.find<world::PowerConsumer>(), nullptr);
}

TEST(BuildingType, PipeRoundTripsThroughItsSaveKey) {
    // Given
    const auto key = world::building_type_key(BuildingType::Pipe);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "Pipe");
    EXPECT_EQ(parsed, BuildingType::Pipe);
}

TEST(PipeNetwork, TouchingPipesFormOneGroupAndAGapSplitsThem) {
    // Given
    Fixture f;
    const auto a = f.add(BuildingType::Pipe, {0, 0});
    const auto b = f.add(BuildingType::Pipe, {1, 0});
    const auto c = f.add(BuildingType::Pipe, {3, 0});
    world::PipeNetwork network;

    // When
    network.rebuild(f.registry);

    // Then
    ASSERT_EQ(network.groups().size(), 2u);
    const auto& first = network.groups()[0].pipes;
    EXPECT_TRUE(contains(first, a) && contains(first, b));
    EXPECT_EQ(network.groups()[1].pipes, std::vector<core::EntityId>{c});
}

TEST(PipeNetwork, TanksBesideAPipeJoinItsGroupButNotDiagonalOnes) {
    // Given: a 2x2 storage on (0..1, 0..1), a pipe on (2, 0), a pump diagonal on (3..4, 1..2)
    Fixture f;
    const auto storage = f.add(BuildingType::LiquidStorage, {0, 0});
    f.add(BuildingType::Pipe, {2, 0});
    f.add(BuildingType::OilPump, {3, 1});
    world::PipeNetwork network;

    // When
    network.rebuild(f.registry);

    // Then
    ASSERT_EQ(network.groups().size(), 1u);
    EXPECT_EQ(network.groups()[0].members, std::vector<core::EntityId>{storage});
}

TEST(PipeNetwork, AnUnfinishedPipeConnectsNothing) {
    // Given
    Fixture f;
    f.add(BuildingType::LiquidStorage, {0, 0});
    f.site(BuildingType::Pipe, {2, 0});
    world::PipeNetwork network;

    // When
    network.rebuild(f.registry);

    // Then
    EXPECT_TRUE(network.groups().empty());
}

TEST(PipeFlowSystem, APumpPipedToAStorageMovesAtMost50UnitsPerSecond) {
    // Given: storage (0..1), pipes (2, 0) and (3, 0), pump (4..5)
    Fixture f;
    const auto storage = f.add(BuildingType::LiquidStorage, {0, 0});
    f.add(BuildingType::Pipe, {2, 0});
    f.add(BuildingType::Pipe, {3, 0});
    const auto pump = f.add(BuildingType::OilPump, {4, 0});
    f.tank(pump).add(world::items::Oil, 500);
    world::PipeFlowSystem system{f.registry, f.bus};

    // When
    system.update(1.0f);

    // Then
    EXPECT_EQ(f.tank(storage).count(world::items::Oil), 50);
    EXPECT_EQ(f.tank(pump).count(world::items::Oil), 450);
}

TEST(PipeFlowSystem, NothingFlowsWithoutAPipeBetweenTheTanks) {
    // Given: a gap at (3, 0)
    Fixture f;
    const auto storage = f.add(BuildingType::LiquidStorage, {0, 0});
    f.add(BuildingType::Pipe, {2, 0});
    const auto pump = f.add(BuildingType::OilPump, {4, 0});
    f.tank(pump).add(world::items::Oil, 500);
    world::PipeFlowSystem system{f.registry, f.bus};

    // When
    system.update(1.0f);

    // Then
    EXPECT_EQ(f.tank(storage).count(world::items::Oil), 0);
    EXPECT_EQ(f.tank(pump).count(world::items::Oil), 500);
}

TEST(PipeFlowSystem, AFullStorageLeavesTheOilInThePump) {
    // Given
    Fixture f;
    const auto storage = f.add(BuildingType::LiquidStorage, {0, 0});
    f.add(BuildingType::Pipe, {2, 0});
    f.add(BuildingType::Pipe, {3, 0});
    const auto pump = f.add(BuildingType::OilPump, {4, 0});
    f.tank(storage).add(world::items::Oil, f.tank(storage).capacity());
    f.tank(pump).add(world::items::Oil, 500);
    world::PipeFlowSystem system{f.registry, f.bus};

    // When
    system.update(1.0f);

    // Then
    EXPECT_EQ(f.tank(pump).count(world::items::Oil), 500);
}

TEST(PipeFlowSystem, OilSkipsAStorageHoldingDieselAndFillsTheOtherOne) {
    // Given: one pipe line (2..8, 2) under a pump, with two storages below it, one holding diesel
    Fixture f;
    for (int x = 2; x < 9; ++x) {
        f.add(BuildingType::Pipe, {x, 2});
    }
    const auto pump = f.add(BuildingType::OilPump, {2, 0});
    const auto diesel = f.add(BuildingType::LiquidStorage, {2, 3});
    const auto empty = f.add(BuildingType::LiquidStorage, {6, 3});
    f.tank(diesel).add(world::items::Diesel, 10);
    f.tank(pump).add(world::items::Oil, 500);
    world::PipeFlowSystem system{f.registry, f.bus};

    // When
    system.update(1.0f);

    // Then
    EXPECT_EQ(f.tank(diesel).count(world::items::Oil), 0);
    EXPECT_EQ(f.tank(empty).count(world::items::Oil), 50);
}

TEST(PipeFlowSystem, SeveralProducersAndConsumersShareTheFlowEvenly) {
    // Given: a pipe line on y = 2 from x = 0 to 7, two pumps above it, two storages below it
    Fixture f;
    for (int x = 0; x < 8; ++x) {
        f.add(BuildingType::Pipe, {x, 2});
    }
    const auto pump_a = f.add(BuildingType::OilPump, {0, 0});
    const auto pump_b = f.add(BuildingType::OilPump, {4, 0});
    const auto storage_a = f.add(BuildingType::LiquidStorage, {0, 3});
    const auto storage_b = f.add(BuildingType::LiquidStorage, {4, 3});
    f.tank(pump_a).add(world::items::Oil, 500);
    f.tank(pump_b).add(world::items::Oil, 500);
    world::PipeFlowSystem system{f.registry, f.bus};

    // When
    system.update(2.0f);

    // Then: 100 units moved in total, 50 from and to each
    EXPECT_EQ(f.tank(pump_a).count(world::items::Oil), 450);
    EXPECT_EQ(f.tank(pump_b).count(world::items::Oil), 450);
    EXPECT_EQ(f.tank(storage_a).count(world::items::Oil), 50);
    EXPECT_EQ(f.tank(storage_b).count(world::items::Oil), 50);
}

TEST(PipeFlowSystem, ANewPipeConnectsOnceItIsCompleted) {
    // Given: the gap at (3, 0) filled after the first update
    Fixture f;
    const auto storage = f.add(BuildingType::LiquidStorage, {0, 0});
    f.add(BuildingType::Pipe, {2, 0});
    const auto pump = f.add(BuildingType::OilPump, {4, 0});
    f.tank(pump).add(world::items::Oil, 500);
    world::PipeFlowSystem system{f.registry, f.bus};
    system.update(1.0f);

    // When
    const auto pipe = f.add(BuildingType::Pipe, {3, 0});
    f.bus.publish(world::BuildingCompleted{pipe});
    system.update(1.0f);

    // Then
    EXPECT_EQ(f.tank(storage).count(world::items::Oil), 50);
}
