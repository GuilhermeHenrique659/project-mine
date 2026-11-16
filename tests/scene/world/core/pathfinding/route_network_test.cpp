#include <gtest/gtest.h>

#include <memory>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

using world::TileCoord;

// Everything here stays inside the spawn clearing, which is always grass.
struct Fixture {
    core::Registry registry;
    world::Terrain terrain{7};
    world::RouteNetwork network{std::make_unique<world::HeadquartersRoutePlanner>()};

    core::EntityId add(world::BuildingType type, TileCoord tile) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, tile});
        return id;
    }
};

} // namespace

TEST(RouteNetwork, HeadquartersGetsOneRouteToEveryOtherBuilding) {
    // Given
    Fixture f;
    const auto hq = f.add(world::BuildingType::Headquarters, {0, 0});
    const auto mine_a = f.add(world::BuildingType::Mine, {5, 0});
    const auto mine_b = f.add(world::BuildingType::Mine, {0, -6});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    EXPECT_EQ(f.network.routes().size(), 2u);
    const world::Route* to_a = f.network.find({hq, mine_a});
    const world::Route* to_b = f.network.find({hq, mine_b});
    ASSERT_NE(to_a, nullptr);
    ASSERT_NE(to_b, nullptr);
    EXPECT_EQ(to_a->tiles.front(), (TileCoord{0, 0}));
    EXPECT_EQ(to_a->tiles.back(), (TileCoord{5, 0}));
    EXPECT_EQ(to_b->tiles.back(), (TileCoord{0, -6}));
}

TEST(RouteNetwork, WithoutHeadquartersThereAreNoRoutes) {
    // Given
    Fixture f;
    f.add(world::BuildingType::Mine, {3, 0});
    f.add(world::BuildingType::Mine, {6, 0});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    EXPECT_TRUE(f.network.routes().empty());
}

TEST(RouteNetwork, RouteWalksAroundABuildingInTheWay) {
    // Given: a mine sits between the headquarters and the target
    Fixture f;
    const auto hq = f.add(world::BuildingType::Headquarters, {0, 0});
    f.add(world::BuildingType::Mine, {2, 0});
    const auto target = f.add(world::BuildingType::Mine, {4, 0});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    const world::Route* route = f.network.find({hq, target});
    ASSERT_NE(route, nullptr);
    ASSERT_TRUE(route->reachable());
    for (const TileCoord tile : route->tiles) {
        EXPECT_FALSE(tile == (TileCoord{2, 0}));
    }
}

TEST(RouteNetwork, UnreachableBuildingKeepsAnEmptyRoute) {
    // Given: the target is walled in by eight other buildings
    Fixture f;
    const auto hq = f.add(world::BuildingType::Headquarters, {0, 0});
    const auto target = f.add(world::BuildingType::Mine, {6, 6});
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx != 0 || dy != 0) {
                f.add(world::BuildingType::Mine, {6 + dx, 6 + dy});
            }
        }
    }
    world::RouteNetwork network{std::make_unique<world::HeadquartersRoutePlanner>(), 500};

    // When
    network.rebuild(f.registry, f.terrain);

    // Then
    const world::Route* route = network.find({hq, target});
    ASSERT_NE(route, nullptr);
    EXPECT_FALSE(route->reachable());
}

TEST(RouteNetwork, EachRebuildBumpsTheRevision) {
    // Given
    Fixture f;
    f.add(world::BuildingType::Headquarters, {0, 0});
    f.network.rebuild(f.registry, f.terrain);
    const auto before = f.network.revision();

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    EXPECT_GT(f.network.revision(), before);
}

TEST(RouteNetwork, RoutesAvoidEveryTileOfAMultiTileBuildingInTheWay) {
    // Given: a 1x5 wall-like building between the headquarters and the target
    Fixture f;
    const auto hq = f.add(world::BuildingType::Headquarters, {0, 0});
    const core::EntityId wall = f.registry.create("wall");
    f.registry.add(wall, world::Building{world::BuildingType::Mine, {2, -2}, {1, 5}});
    const auto target = f.add(world::BuildingType::Mine, {4, 0});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    const world::Route* route = f.network.find({hq, target});
    ASSERT_NE(route, nullptr);
    ASSERT_TRUE(route->reachable());
    for (const TileCoord tile : route->tiles) {
        EXPECT_FALSE(tile.x == 2 && tile.y >= -2 && tile.y <= 2);
    }
}

TEST(RouteNetwork, ARouteLeavesAMultiTileBuildingFromAnyOfItsSides) {
    // Given: a 3x3 headquarters whose origin corner is walled in by other buildings
    Fixture f;
    const core::EntityId hq = f.registry.create("hq");
    f.registry.add(hq, world::Building{world::BuildingType::Headquarters, {0, 0}, {3, 3}});
    f.add(world::BuildingType::Mine, {-1, -1});
    f.add(world::BuildingType::Mine, {-1, 0});
    f.add(world::BuildingType::Mine, {0, -1});
    f.add(world::BuildingType::Mine, {1, -1});
    f.add(world::BuildingType::Mine, {-1, 1});
    const auto target = f.add(world::BuildingType::Mine, {6, 6});

    // When
    f.network.rebuild(f.registry, f.terrain);

    // Then
    const world::Route* route = f.network.find({hq, target});
    ASSERT_NE(route, nullptr);
    EXPECT_TRUE(route->reachable());
}
