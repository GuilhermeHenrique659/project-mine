#include <gtest/gtest.h>

#include <cstdlib>
#include <optional>
#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/entities/battle_tank.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/entities/vehicle.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/events/vehicle_events.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/systems/vehicle_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

// The spawn clearing around the origin is dry land.
struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::VehicleSystem system{registry, bus, terrain};
    std::optional<world::UnitsApproachRequested> approach;
    event::ScopedSubscription spy = event::subscribe_scoped<world::UnitsApproachRequested>(
        bus, [this](const world::UnitsApproachRequested& request) { approach = request; });

    Fixture() { world::track_buildings(registry); }

    world::WorldPoint at(core::EntityId id) const { return registry.get<world::Position>(id)->point; }
};

} // namespace

TEST(VehicleSystem, PassengersRideAlongWithTheirVehicle) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {0.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    world::board(f.registry, tank, soldier);
    f.registry.get<world::Position>(tank)->point = {4.5f, 2.5f};

    // When
    f.system.update(0.016f);

    // Then
    EXPECT_FLOAT_EQ(f.at(soldier).x, 4.5f);
    EXPECT_FLOAT_EQ(f.at(soldier).y, 2.5f);
}

TEST(VehicleSystem, ACharacterWithinReachGetsIn) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {1.5f, 0.5f});
    const core::EntityId player = world::spawn_character(f.registry, "Player", {0.0f, 0.0f});

    // When
    f.bus.publish(world::VehicleBoardRequested{player, tank});

    // Then
    ASSERT_NE(f.registry.get<world::Passenger>(player), nullptr);
    EXPECT_EQ(f.registry.get<world::Passenger>(player)->vehicle, tank);
}

TEST(VehicleSystem, ACharacterOutOfReachStaysOut) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {6.5f, 0.5f});
    const core::EntityId player = world::spawn_character(f.registry, "Player", {0.0f, 0.0f});

    // When
    f.bus.publish(world::VehicleBoardRequested{player, tank});

    // Then
    EXPECT_FALSE(f.registry.has<world::Passenger>(player));
}

TEST(VehicleSystem, GettingOutPutsThePassengerOnAFreeTileBesideTheVehicle) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {2.5f, 2.5f});
    const core::EntityId player = world::spawn_character(f.registry, "Player", {2.5f, 2.5f});
    world::board(f.registry, tank, player);

    // When
    f.bus.publish(world::VehicleExitRequested{player});

    // Then: out, on a tile next to the tank's, not on it
    EXPECT_FALSE(f.registry.has<world::Passenger>(player));
    const world::TileCoord tile = world::tile_containing(f.at(player));
    EXPECT_NE(tile, (world::TileCoord{2, 2}));
    EXPECT_LE(std::abs(tile.x - 2), 1);
    EXPECT_LE(std::abs(tile.y - 2), 1);
}

TEST(VehicleSystem, AUnitOrderedIntoAVehicleWalksToIt) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {6.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    f.bus.publish(world::UnitsBoardRequested{{soldier}, tank});

    // When
    f.system.update(0.016f);

    // Then
    ASSERT_TRUE(f.approach.has_value());
    EXPECT_EQ(f.approach->units, std::vector<core::EntityId>{soldier});
    EXPECT_EQ(f.approach->goal, (world::TileCoord{6, 0}));
    EXPECT_FALSE(f.registry.has<world::Passenger>(soldier));
}

TEST(VehicleSystem, AnOrderedUnitGetsInOnceItReachesTheVehicle) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {6.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    f.bus.publish(world::UnitsBoardRequested{{soldier}, tank});
    f.system.update(0.016f);
    f.registry.get<world::Position>(soldier)->point = {5.5f, 0.5f};

    // When
    f.system.update(0.016f);

    // Then
    EXPECT_TRUE(f.registry.has<world::Passenger>(soldier));
    EXPECT_FALSE(f.registry.has<world::BoardOrder>(soldier));
}

TEST(VehicleSystem, AUnitReachingAFullVehicleGivesUp) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {1.5f, 0.5f});
    for (std::size_t i = 0; i < world::BattleTankSeats; ++i) {
        world::board(f.registry, tank, world::spawn_soldier(f.registry, "S", {1.5f, 0.5f}));
    }
    const core::EntityId late = world::spawn_soldier(f.registry, "Late", {0.5f, 0.5f});
    f.bus.publish(world::UnitsBoardRequested{{late}, tank});

    // When
    f.system.update(0.016f);

    // Then
    EXPECT_FALSE(f.registry.has<world::Passenger>(late));
    EXPECT_FALSE(f.registry.has<world::BoardOrder>(late));
}

TEST(VehicleSystem, AMoveOrderCancelsBoarding) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {6.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    f.bus.publish(world::UnitsBoardRequested{{soldier}, tank});
    ASSERT_TRUE(f.registry.has<world::BoardOrder>(soldier));

    // When
    f.bus.publish(world::UnitsMoveRequested{{soldier}, {0, 4}});

    // Then
    EXPECT_FALSE(f.registry.has<world::BoardOrder>(soldier));
}

TEST(VehicleSystem, PassengersOfADestroyedVehicleAreLetOut) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {0.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {0.5f, 0.5f});
    world::board(f.registry, tank, soldier);
    f.registry.destroy(tank);

    // When
    f.system.update(0.016f);

    // Then
    EXPECT_FALSE(f.registry.has<world::Passenger>(soldier));
}

TEST(VehicleSystem, UnloadingLetsEveryUnitOutOnItsOwnTileButKeepsThePlayer) {
    // Given
    Fixture f;
    const core::EntityId tank = world::spawn_battle_tank(f.registry, "Tank", {2.5f, 2.5f});
    const core::EntityId first = world::spawn_soldier(f.registry, "A", {2.5f, 2.5f});
    const core::EntityId second = world::spawn_soldier(f.registry, "B", {2.5f, 2.5f});
    const core::EntityId player = world::spawn_character(f.registry, "Player", {2.5f, 2.5f});
    f.registry.add(player, world::PlayerControlled{});
    world::board(f.registry, tank, first);
    world::board(f.registry, tank, second);
    world::board(f.registry, tank, player);

    // When
    f.bus.publish(world::VehicleUnloadRequested{tank});

    // Then
    EXPECT_FALSE(f.registry.has<world::Passenger>(first));
    EXPECT_FALSE(f.registry.has<world::Passenger>(second));
    EXPECT_TRUE(f.registry.has<world::Passenger>(player));
    EXPECT_EQ(f.registry.get<world::Vehicle>(tank)->passengers, std::vector<core::EntityId>{player});
    EXPECT_NE(world::tile_containing(f.at(first)), world::tile_containing(f.at(second)));
    EXPECT_NE(world::tile_containing(f.at(first)), (world::TileCoord{2, 2}));
}
