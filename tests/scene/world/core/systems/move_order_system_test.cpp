#include <gtest/gtest.h>

#include <algorithm>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/systems/move_order_system.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/entities/battle_tank.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/entities/vehicle.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

// The spawn clearing around the origin is dry land.
struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::MoveOrderSystem system{registry, bus, terrain};

    Fixture() { world::track_buildings(registry); }

    const world::PathFollower* path(core::EntityId id) const {
        return registry.get<world::PathFollower>(id);
    }
};

bool passes(const std::vector<world::TileCoord>& tiles, world::TileCoord tile) {
    return std::find(tiles.begin(), tiles.end(), tile) != tiles.end();
}

} // namespace

TEST(MoveOrderSystem, AnOrderedSoldierGetsAPathFromItsTileToTheGoal) {
    // Given
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsMoveRequested{{soldier}, {4, 0}});

    // Then
    ASSERT_NE(f.path(soldier), nullptr);
    EXPECT_EQ(f.path(soldier)->tiles.front(), (world::TileCoord{0, 0}));
    EXPECT_EQ(f.path(soldier)->tiles.back(), (world::TileCoord{4, 0}));
}

TEST(MoveOrderSystem, ThePathGoesAroundABuilding) {
    // Given
    Fixture f;
    const core::EntityId wall = f.registry.create("building");
    f.registry.add(wall, world::Building{world::BuildingType::Mine, {2, 0}});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsMoveRequested{{soldier}, {4, 0}});

    // Then
    ASSERT_NE(f.path(soldier), nullptr);
    EXPECT_FALSE(passes(f.path(soldier)->tiles, {2, 0}));
    EXPECT_EQ(f.path(soldier)->tiles.back(), (world::TileCoord{4, 0}));
}

TEST(MoveOrderSystem, SoldiersOrderedTogetherGoToDifferentTiles) {
    // Given
    Fixture f;
    const core::EntityId first = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    const core::EntityId second = world::spawn_soldier(f.registry, "Soldier", {0.5f, 1.5f});

    // When
    f.bus.publish(world::UnitsMoveRequested{{first, second}, {5, 5}});

    // Then
    ASSERT_NE(f.path(first), nullptr);
    ASSERT_NE(f.path(second), nullptr);
    EXPECT_FALSE(f.path(first)->tiles.back() == f.path(second)->tiles.back());
}

TEST(MoveOrderSystem, ANewOrderReplacesTheOldPath) {
    // Given
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    f.bus.publish(world::UnitsMoveRequested{{soldier}, {4, 0}});

    // When
    f.bus.publish(world::UnitsMoveRequested{{soldier}, {0, 4}});

    // Then
    ASSERT_NE(f.path(soldier), nullptr);
    EXPECT_EQ(f.path(soldier)->tiles.back(), (world::TileCoord{0, 4}));
    EXPECT_EQ(f.path(soldier)->waypoint, 0u);
}

TEST(MoveOrderSystem, AWorkerInTheOrderIsIgnored) {
    // Given
    Fixture f;
    const core::EntityId worker = world::spawn_worker(f.registry, "Worker", {0.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsMoveRequested{{worker}, {4, 0}});

    // Then
    EXPECT_EQ(f.path(worker), nullptr);
}

TEST(MoveOrderSystem, AnApproachGivesTheSoldierAPathTowardsTheGoal) {
    // Given
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsApproachRequested{{soldier}, {4, 0}});

    // Then
    ASSERT_NE(f.path(soldier), nullptr);
    EXPECT_EQ(f.path(soldier)->tiles.back(), (world::TileCoord{4, 0}));
}

TEST(MoveOrderSystem, ACrewedTankIsOrderedLikeASoldier) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {0.5f, 0.5f});
    world::board(f.registry, tank, world::spawn_soldier(f.registry, "Crew", {0.5f, 0.5f}));

    // When
    f.bus.publish(world::UnitsMoveRequested{{tank}, {4, 0}});

    // Then
    ASSERT_NE(f.path(tank), nullptr);
    EXPECT_EQ(f.path(tank)->tiles.back(), (world::TileCoord{4, 0}));
}

TEST(MoveOrderSystem, AnEmptyTankIgnoresOrders) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {0.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsMoveRequested{{tank}, {4, 0}});

    // Then
    EXPECT_EQ(f.path(tank), nullptr);
}

TEST(MoveOrderSystem, ATankThePlayerDrivesIgnoresOrders) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {0.5f, 0.5f});
    const core::EntityId player = world::spawn_character(f.registry, "Player", {0.5f, 0.5f});
    f.registry.add(player, world::PlayerControlled{});
    world::board(f.registry, tank, player);

    // When
    f.bus.publish(world::UnitsMoveRequested{{tank}, {4, 0}});

    // Then
    EXPECT_EQ(f.path(tank), nullptr);
}

TEST(MoveOrderSystem, ASoldierInsideAVehicleDoesNotWalkAway) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {0.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    world::board(f.registry, tank, soldier);

    // When
    f.bus.publish(world::UnitsMoveRequested{{soldier}, {4, 0}});

    // Then
    EXPECT_EQ(f.path(soldier), nullptr);
}
