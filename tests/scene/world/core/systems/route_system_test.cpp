#include <gtest/gtest.h>

#include <memory>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/systems/route_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::RouteNetwork network{std::make_unique<world::HeadquartersRoutePlanner>()};

    core::EntityId add(world::BuildingType type, world::TileCoord tile) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, tile});
        return id;
    }
};

} // namespace

TEST(RouteSystem, FirstUpdateComputesTheRoutesOfTheBuildingsAlreadyThere) {
    // Given: buildings restored from a save, before any system ran
    Fixture f;
    f.add(world::BuildingType::Headquarters, {0, 0});
    f.add(world::BuildingType::Mine, {4, 0});
    world::RouteSystem system(f.registry, f.bus, f.terrain, f.network);

    // When
    system.update(0.016f);

    // Then
    EXPECT_EQ(f.network.routes().size(), 1u);
}

TEST(RouteSystem, RoutesAreNotRecomputedEveryFrame) {
    // Given
    Fixture f;
    f.add(world::BuildingType::Headquarters, {0, 0});
    world::RouteSystem system(f.registry, f.bus, f.terrain, f.network);
    system.update(0.016f);
    const auto revision = f.network.revision();

    // When
    system.update(0.016f);
    system.update(0.016f);

    // Then
    EXPECT_EQ(f.network.revision(), revision);
}

TEST(RouteSystem, APlacedBuildingGetsItsRouteOnTheNextUpdate) {
    // Given
    Fixture f;
    const auto hq = f.add(world::BuildingType::Headquarters, {0, 0});
    world::RouteSystem system(f.registry, f.bus, f.terrain, f.network);
    system.update(0.016f);
    const auto mine = f.add(world::BuildingType::Mine, {0, 5});

    // When
    f.bus.publish(world::BuildingPlaced{{0, 5}});
    system.update(0.016f);

    // Then
    const world::Route* route = f.network.find({hq, mine});
    ASSERT_NE(route, nullptr);
    EXPECT_TRUE(route->reachable());
}

TEST(RouteSystem, SeveralPlacementsInOneFrameRecomputeOnce) {
    // Given
    Fixture f;
    f.add(world::BuildingType::Headquarters, {0, 0});
    world::RouteSystem system(f.registry, f.bus, f.terrain, f.network);
    system.update(0.016f);
    const auto revision = f.network.revision();

    // When
    f.bus.publish(world::BuildingPlaced{{0, 5}});
    f.bus.publish(world::BuildingPlaced{{0, 7}});
    system.update(0.016f);

    // Then
    EXPECT_EQ(f.network.revision(), revision + 1);
}
