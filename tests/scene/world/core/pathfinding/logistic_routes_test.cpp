#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

using world::TileCoord;

std::unique_ptr<world::RoutePlanner> hq_and_logistics() {
    std::vector<std::unique_ptr<world::RoutePlanner>> planners;
    planners.push_back(std::make_unique<world::HeadquartersRoutePlanner>());
    planners.push_back(std::make_unique<world::LogisticRoutePlanner>());
    return std::make_unique<world::CompositeRoutePlanner>(std::move(planners));
}

// Everything here stays inside the spawn clearing, which is always grass.
struct Fixture {
    core::Registry registry;
    world::Terrain terrain{7};
    world::RouteNetwork network{hq_and_logistics()};

    core::EntityId add(world::BuildingType type, TileCoord tile) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, tile});
        return id;
    }

    core::EntityId add_producer(TileCoord tile) {
        const core::EntityId id = add(world::BuildingType::Mine, tile);
        registry.add(id, world::Producer{});
        return id;
    }
};

} // namespace

TEST(LogisticRoutes, EveryStorageGetsALogisticRouteToEveryProductionBuilding) {
    // Given
    Fixture f;
    const auto storage = f.add(world::BuildingType::Storage, {0, 0});
    const auto mine_a = f.add_producer({5, 0});
    const auto mine_b = f.add_producer({0, -6});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    const world::Route* to_a = f.network.find({storage, mine_a});
    const world::Route* to_b = f.network.find({storage, mine_b});
    ASSERT_NE(to_a, nullptr);
    ASSERT_NE(to_b, nullptr);
    EXPECT_EQ(to_a->kind, world::RouteKind::Logistic);
    EXPECT_EQ(to_b->kind, world::RouteKind::Logistic);
    EXPECT_EQ(to_a->tiles.back(), (TileCoord{5, 0}));
}

TEST(LogisticRoutes, StoragesGetALogisticRouteToFuelledGeneratorsButNotToSolarOnes) {
    // Given
    Fixture f;
    const auto storage = f.add(world::BuildingType::Storage, {0, 0});
    const auto coal = f.add(world::BuildingType::CoalGenerator, {5, 0});
    f.registry.add(coal, world::Generator{{world::GeneratorType::Coal,
                                           world::ItemStack{world::items::Coal, 1}, 1.0f,
                                           1000.0f}});
    const auto solar = f.add(world::BuildingType::Headquarters, {0, 6});
    f.registry.add(solar, world::Generator{});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    ASSERT_NE(f.network.find({storage, coal}), nullptr);
    EXPECT_EQ(f.network.find({storage, coal})->kind, world::RouteKind::Logistic);
    EXPECT_EQ(f.network.find({storage, solar}), nullptr);
}

TEST(LogisticRoutes, BuildingsThatDoNotProduceGetNoLogisticRoute) {
    // Given
    Fixture f;
    const auto storage = f.add(world::BuildingType::Storage, {0, 0});
    const auto other_storage = f.add(world::BuildingType::Storage, {5, 0});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    EXPECT_EQ(f.network.find({storage, other_storage}), nullptr);
    EXPECT_TRUE(f.network.routes().empty());
}

TEST(LogisticRoutes, HeadquartersRoutesStayWorkerRoutesNextToTheLogisticOnes) {
    // Given
    Fixture f;
    const auto hq = f.add(world::BuildingType::Headquarters, {0, 0});
    const auto storage = f.add(world::BuildingType::Storage, {6, 0});
    const auto mine = f.add_producer({0, 6});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    EXPECT_EQ(f.network.routes().size(), 3u); // hq->storage, hq->mine, storage->mine
    ASSERT_NE(f.network.find({hq, mine}), nullptr);
    EXPECT_EQ(f.network.find({hq, mine})->kind, world::RouteKind::Worker);
    ASSERT_NE(f.network.find({storage, mine}), nullptr);
    EXPECT_EQ(f.network.find({storage, mine})->kind, world::RouteKind::Logistic);
}
