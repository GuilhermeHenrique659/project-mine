#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/systems/route_follow_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

constexpr float Frame = 0.05f;

// Headquarters on (0,0) and a mine on (3,0): the route is the straight row (0,0)..(3,0).
struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::RouteNetwork network{std::make_unique<world::HeadquartersRoutePlanner>()};
    core::EntityId hq = add_building(world::BuildingType::Headquarters, {0, 0});
    core::EntityId mine = add_building(world::BuildingType::Mine, {3, 0});
    world::RouteFollowSystem system{registry, network};

    Fixture() { network.rebuild(registry, terrain); }

    core::EntityId add_building(world::BuildingType type, world::TileCoord tile) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, tile});
        return id;
    }

    core::EntityId add_walker(world::WorldPoint at, world::RouteEnd end) {
        const core::EntityId id = registry.create("walker");
        registry.add(id, world::Character{});
        registry.add(id, world::Position{at});
        registry.add(id, world::Mover{2.0f, {}, false});
        registry.add(id, world::RouteFollower{{hq, mine}, end});
        return id;
    }

    const world::WorldPoint& direction(core::EntityId id) {
        return registry.get<world::Mover>(id)->direction;
    }
};

} // namespace

TEST(RouteFollowSystem, AWalkerAtTheStartHeadsToTheNextWaypoint) {
    // Given
    Fixture f;
    const auto walker = f.add_walker({0.5f, 0.5f}, world::RouteEnd::Stop);

    // When
    f.system.update(Frame);

    // Then
    EXPECT_GT(f.direction(walker).x, 0.0f);
    EXPECT_FLOAT_EQ(f.direction(walker).y, 0.0f);
}

TEST(RouteFollowSystem, StopModeStandsStillAtTheEndOfTheRoute) {
    // Given
    Fixture f;
    const auto walker = f.add_walker({3.5f, 0.5f}, world::RouteEnd::Stop);

    // When
    f.system.update(Frame);

    // Then
    EXPECT_FLOAT_EQ(f.direction(walker).x, 0.0f);
    EXPECT_FLOAT_EQ(f.direction(walker).y, 0.0f);
}

TEST(RouteFollowSystem, LoopModeTurnsBackAtTheEndOfTheRoute) {
    // Given
    Fixture f;
    const auto walker = f.add_walker({3.5f, 0.5f}, world::RouteEnd::Loop);

    // When
    f.system.update(Frame);

    // Then
    EXPECT_LT(f.direction(walker).x, 0.0f);
}

TEST(RouteFollowSystem, LoopModeTurnsForwardAgainAtTheStart) {
    // Given: walking back towards the headquarters
    Fixture f;
    const auto walker = f.add_walker({0.5f, 0.5f}, world::RouteEnd::Loop);
    f.system.update(Frame);
    auto& follower = *f.registry.get<world::RouteFollower>(walker);
    follower.forward = false;
    follower.waypoint = 0;

    // When
    f.system.update(Frame);

    // Then
    EXPECT_GT(f.direction(walker).x, 0.0f);
    EXPECT_TRUE(follower.forward);
}

TEST(RouteFollowSystem, WithoutItsRouteAWalkerStandsStill) {
    // Given: the follower asks for a pair the planner never routes
    Fixture f;
    const auto walker = f.add_walker({1.5f, 0.5f}, world::RouteEnd::Stop);
    f.registry.get<world::RouteFollower>(walker)->route = {f.mine, f.hq};
    f.registry.get<world::Mover>(walker)->direction = {1.0f, 0.0f};

    // When
    f.system.update(Frame);

    // Then
    EXPECT_FLOAT_EQ(f.direction(walker).x, 0.0f);
}

TEST(RouteFollowSystem, WalksTheWholeRouteAndStopsOnTheLastTile) {
    // Given
    Fixture f;
    const auto walker = f.add_walker({0.5f, 0.5f}, world::RouteEnd::Stop);
    world::CharacterMoveSystem move{f.registry, f.bus};

    // When: 5 s at 2 tiles/s covers the 3-tile route with time to spare
    for (int frame = 0; frame < 100; ++frame) {
        f.system.update(Frame);
        move.update(Frame);
    }

    // Then
    const world::WorldPoint at = f.registry.get<world::Position>(walker)->point;
    EXPECT_NEAR(at.x, 3.5f, 0.11f);
    EXPECT_NEAR(at.y, 0.5f, 0.11f);
    EXPECT_FLOAT_EQ(f.direction(walker).x, 0.0f);
}

TEST(RouteFollowSystem, AfterARecalculationResumesFromTheNearestWaypoint) {
    // Given: walking the route, then a building is placed right in front of the walker
    Fixture f;
    const auto walker = f.add_walker({1.5f, 0.5f}, world::RouteEnd::Stop);
    f.system.update(Frame);
    f.add_building(world::BuildingType::Mine, {2, 0});
    f.network.rebuild(f.registry, f.terrain);

    // When
    f.system.update(Frame);

    // Then: the next waypoint is on the new route and next to the walker
    const world::Route* route = f.network.find({f.hq, f.mine});
    ASSERT_NE(route, nullptr);
    const auto& follower = *f.registry.get<world::RouteFollower>(walker);
    EXPECT_EQ(follower.revision, f.network.revision());
    EXPECT_GT(follower.waypoint, 0u); // not sent back to the headquarters
    ASSERT_LT(follower.waypoint, route->tiles.size());
    const world::TileCoord next = route->tiles[follower.waypoint];
    EXPECT_FALSE(next == (world::TileCoord{2, 0}));
    EXPECT_LE(std::hypot(next.x + 0.5f - 1.5f, next.y + 0.5f - 0.5f), 1.5f);
}
