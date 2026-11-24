#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/selected.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/entities/battle_tank.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/entities/vehicle.hpp"
#include "scene/world/core/targeting.hpp"

namespace {

std::size_t passenger_count(const core::Registry& registry, core::EntityId vehicle) {
    return registry.get<world::Vehicle>(vehicle)->passengers.size();
}

} // namespace

TEST(Vehicle, ABattleTankSeatsSixAndRefusesTheSeventh) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});
    for (int i = 0; i < 6; ++i) {
        ASSERT_TRUE(world::board(registry, tank, world::spawn_soldier(registry, "S", {0.5f, 0.5f})));
    }
    const core::EntityId seventh = world::spawn_soldier(registry, "S", {0.5f, 0.5f});

    // When
    const bool boarded = world::board(registry, tank, seventh);

    // Then
    EXPECT_FALSE(boarded);
    EXPECT_EQ(passenger_count(registry, tank), world::BattleTankSeats);
    EXPECT_FALSE(registry.has<world::Passenger>(seventh));
}

TEST(Vehicle, BoardingSeatsThePassengerAndDropsItsSelection) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(registry, "Soldier", {0.5f, 0.5f});
    registry.add(soldier, world::Selected{});

    // When
    world::board(registry, tank, soldier);

    // Then
    ASSERT_NE(registry.get<world::Passenger>(soldier), nullptr);
    EXPECT_EQ(registry.get<world::Passenger>(soldier)->vehicle, tank);
    EXPECT_TRUE(world::is_crewed(registry, tank));
    EXPECT_FALSE(registry.has<world::Selected>(soldier));
}

TEST(Vehicle, APassengerCannotBoardASecondVehicle) {
    // Given
    core::Registry registry;
    const core::EntityId first = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});
    const core::EntityId second = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(registry, "Soldier", {0.5f, 0.5f});
    world::board(registry, first, soldier);

    // When
    const bool boarded = world::board(registry, second, soldier);

    // Then
    EXPECT_FALSE(boarded);
    EXPECT_EQ(passenger_count(registry, second), 0u);
}

TEST(Vehicle, AVehicleCannotBoardAnotherVehicle) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});
    const core::EntityId other = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});

    // When
    const bool boarded = world::board(registry, tank, other);

    // Then
    EXPECT_FALSE(boarded);
    EXPECT_EQ(passenger_count(registry, tank), 0u);
}

TEST(Vehicle, LeavingFreesTheSeatAndEmptiesTheCrew) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(registry, "Soldier", {0.5f, 0.5f});
    world::board(registry, tank, soldier);

    // When
    world::leave(registry, soldier);

    // Then
    EXPECT_FALSE(registry.has<world::Passenger>(soldier));
    EXPECT_EQ(passenger_count(registry, tank), 0u);
    EXPECT_FALSE(world::is_crewed(registry, tank));
}

TEST(Vehicle, ATankCarryingThePlayerIsDrivenByThePlayer) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});
    const core::EntityId soldier = world::spawn_soldier(registry, "Soldier", {0.5f, 0.5f});
    const core::EntityId player = world::spawn_soldier(registry, "Player", {0.5f, 0.5f});
    registry.add(player, world::PlayerControlled{});
    world::board(registry, tank, soldier);
    ASSERT_FALSE(world::carries_player(registry, tank));

    // When
    world::board(registry, tank, player);

    // Then
    EXPECT_TRUE(world::carries_player(registry, tank));
}

TEST(Vehicle, ATankIsFoundOnTheTileItStandsOn) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {3.5f, 2.5f});
    world::spawn_soldier(registry, "Soldier", {1.5f, 1.5f});

    // When
    const core::EntityId on_tank = world::vehicle_at(registry, {3, 2});
    const core::EntityId on_soldier = world::vehicle_at(registry, {1, 1});

    // Then
    EXPECT_EQ(on_tank, tank);
    EXPECT_EQ(on_soldier, 0u);
}

TEST(Vehicle, ATankHasItsOwnHealthAndSpeed) {
    // Given / When
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {0.5f, 0.5f});

    // Then
    EXPECT_EQ(registry.get<world::Health>(tank)->current, world::BattleTankHealth);
    EXPECT_FLOAT_EQ(registry.get<world::Mover>(tank)->speed, world::BattleTankSpeed);
}

TEST(Vehicle, ASoldierInsideATankCannotBeTargetedButTheTankCan) {
    // Given
    core::Registry registry;
    const core::EntityId tank = world::spawn_battle_tank(registry, "Tank", {3.5f, 2.5f});
    world::board(registry, tank, world::spawn_soldier(registry, "Crew", {3.5f, 2.5f}));

    // When
    const core::EntityId target = world::shootable_at(registry, {3, 2}, 0);

    // Then
    EXPECT_EQ(target, tank);
}
