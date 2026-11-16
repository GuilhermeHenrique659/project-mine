#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "event/event_bus.hpp"
#include "scene/world/core/events/character_state_changed.hpp"

#include "core/registry.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

core::EntityId spawn_walker(core::Registry& registry, float speed, world::WorldPoint direction) {
    const core::EntityId id = registry.create("walker");
    registry.add(id, world::Character{});
    registry.add(id, world::Position{{0.0f, 0.0f}});
    registry.add(id, world::Mover{speed, direction});
    return id;
}

float distance_from_origin(const world::Position& position) {
    return std::hypot(position.point.x, position.point.y);
}

} // namespace

TEST(CharacterMoveSystem, MovesBySpeedTimesDeltaTimeAlongTheDirection) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {1.0f, 0.0f});

    // When
    system.update(0.5f);

    // Then
    const world::Position* position = registry.get<world::Position>(id);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->point.x, 2.0f);
    EXPECT_FLOAT_EQ(position->point.y, 0.0f);
}

TEST(CharacterMoveSystem, DiagonalMovementIsNotFasterThanStraightMovement) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId diagonal = spawn_walker(registry, 4.0f, {1.0f, 1.0f});
    const core::EntityId straight = spawn_walker(registry, 4.0f, {1.0f, 0.0f});

    // When
    system.update(1.0f);

    // Then
    EXPECT_NEAR(distance_from_origin(*registry.get<world::Position>(straight)), 4.0f, 1e-4f);
    EXPECT_NEAR(distance_from_origin(*registry.get<world::Position>(diagonal)),
                distance_from_origin(*registry.get<world::Position>(straight)), 1e-4f);
}

TEST(CharacterMoveSystem, ZeroDirectionKeepsThePosition) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {0.0f, 0.0f});

    // When
    system.update(1.0f);

    // Then
    const world::Position* position = registry.get<world::Position>(id);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->point.x, 0.0f);
    EXPECT_FLOAT_EQ(position->point.y, 0.0f);
}

TEST(CharacterMoveSystem, EntityWithoutMoverDoesNotMove) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = registry.create("statue");
    registry.add(id, world::Position{{3.0f, 5.0f}});

    // When
    system.update(1.0f);

    // Then
    const world::Position* position = registry.get<world::Position>(id);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->point.x, 3.0f);
    EXPECT_FLOAT_EQ(position->point.y, 5.0f);
}

TEST(CharacterMoveSystem, EntityThatIsNotACharacterDoesNotMove) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = registry.create("cart");
    registry.add(id, world::Position{{3.0f, 5.0f}});
    registry.add(id, world::Mover{4.0f, {1.0f, 0.0f}});

    // When
    system.update(1.0f);

    // Then
    const world::Position* position = registry.get<world::Position>(id);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->point.x, 3.0f);
    EXPECT_FLOAT_EQ(position->point.y, 5.0f);
}

namespace {

// Records the entities whose state change was announced.
struct StateChangeRecorder {
    explicit StateChangeRecorder(event::EventBus& bus)
        : subscription(event::subscribe_scoped<world::CharacterStateChanged>(
              bus, [this](const world::CharacterStateChanged& e) { changed.push_back(e.entity); })) {}

    std::vector<core::EntityId> changed;
    event::ScopedSubscription subscription;
};

world::CharacterState state_of(const core::Registry& registry, core::EntityId id) {
    return registry.get<world::Character>(id)->state;
}

} // namespace

TEST(CharacterMoveSystem, IdleCharacterThatStartsWalkingBecomesMovingAndAnnouncesIt) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    StateChangeRecorder recorder(bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {1.0f, 0.0f});

    // When
    system.update(0.5f);

    // Then
    EXPECT_EQ(state_of(registry, id), world::CharacterState::Moving);
    EXPECT_EQ(recorder.changed, std::vector<core::EntityId>{id});
}

TEST(CharacterMoveSystem, CharacterThatKeepsWalkingDoesNotAnnounceAgain) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {1.0f, 0.0f});
    system.update(0.5f);
    StateChangeRecorder recorder(bus);

    // When
    system.update(0.5f);

    // Then
    EXPECT_EQ(state_of(registry, id), world::CharacterState::Moving);
    EXPECT_TRUE(recorder.changed.empty());
}

TEST(CharacterMoveSystem, MovingCharacterThatStopsBecomesIdleAndAnnouncesIt) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {1.0f, 0.0f});
    system.update(0.5f);
    StateChangeRecorder recorder(bus);

    // When
    registry.get<world::Mover>(id)->direction = {0.0f, 0.0f};
    system.update(0.5f);

    // Then
    EXPECT_EQ(state_of(registry, id), world::CharacterState::Idle);
    EXPECT_EQ(recorder.changed, std::vector<core::EntityId>{id});
}

TEST(CharacterMoveSystem, RunningCharacterCoversRunSpeedMultiplierTimesTheDistance) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {1.0f, 0.0f});
    registry.get<world::Mover>(id)->running = true;

    // When
    system.update(0.5f);

    // Then
    EXPECT_FLOAT_EQ(registry.get<world::Position>(id)->point.x, 2.0f * world::RunSpeedMultiplier);
}

TEST(CharacterMoveSystem, RunningCharacterBecomesRunningAndAnnouncesIt) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {1.0f, 0.0f});
    system.update(0.5f);
    StateChangeRecorder recorder(bus);

    // When
    registry.get<world::Mover>(id)->running = true;
    system.update(0.5f);

    // Then
    EXPECT_EQ(state_of(registry, id), world::CharacterState::Running);
    EXPECT_EQ(recorder.changed, std::vector<core::EntityId>{id});
}

TEST(CharacterMoveSystem, RunningCharacterThatStopsRunningBecomesMovingAgain) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {1.0f, 0.0f});
    registry.get<world::Mover>(id)->running = true;
    system.update(0.5f);

    // When
    registry.get<world::Mover>(id)->running = false;
    system.update(0.5f);

    // Then
    EXPECT_EQ(state_of(registry, id), world::CharacterState::Moving);
}

TEST(CharacterMoveSystem, RunFlagWithoutDirectionStaysIdle) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    StateChangeRecorder recorder(bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {0.0f, 0.0f});
    registry.get<world::Mover>(id)->running = true;

    // When
    system.update(0.5f);

    // Then
    EXPECT_EQ(state_of(registry, id), world::CharacterState::Idle);
    EXPECT_TRUE(recorder.changed.empty());
}

TEST(CharacterMoveSystem, CharacterStandingStillStaysIdleWithoutAnnouncing) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    world::CharacterMoveSystem system(registry, bus);
    StateChangeRecorder recorder(bus);
    const core::EntityId id = spawn_walker(registry, 4.0f, {0.0f, 0.0f});

    // When
    system.update(0.5f);

    // Then
    EXPECT_EQ(state_of(registry, id), world::CharacterState::Idle);
    EXPECT_TRUE(recorder.changed.empty());
}

namespace {

core::EntityId spawn_player(core::Registry& registry, world::WorldPoint at,
                            world::WorldPoint direction) {
    const core::EntityId id = spawn_walker(registry, 1.0f, direction);
    registry.get<world::Position>(id)->point = at;
    registry.add(id, world::PlayerControlled{});
    return id;
}

void place_building(core::Registry& registry, world::TileCoord tile) {
    const core::EntityId id = registry.create("Mine");
    registry.add(id, world::Building{world::BuildingType::Mine, tile, {1, 1}});
}

// A grass tile whose +x neighbour is water.
world::TileCoord shore(const world::Terrain& terrain) {
    for (int x = world::SpawnClearRadius;; ++x) {
        if (terrain.tile_at({x, 0}) == world::TileType::Grass &&
            terrain.tile_at({x + 1, 0}) == world::TileType::Water) {
            return {x, 0};
        }
    }
}

} // namespace

TEST(CharacterMoveSystem, PlayerCannotWalkIntoABuilding) {
    // Given: a building on the tile right of the player
    core::Registry registry;
    event::EventBus bus;
    const world::Terrain terrain(777);
    world::CharacterMoveSystem system(registry, bus, &terrain);
    place_building(registry, {1, 0});
    const core::EntityId id = spawn_player(registry, {0.5f, 0.5f}, {1.0f, 0.0f});

    // When
    for (int step = 0; step < 20; ++step) system.update(0.1f);

    // Then: it stops before the building's edge
    EXPECT_LT(registry.get<world::Position>(id)->point.x, 1.0f);
}

TEST(CharacterMoveSystem, PlayerCannotWalkIntoWater) {
    // Given
    core::Registry registry;
    event::EventBus bus;
    const world::Terrain terrain(777);
    world::CharacterMoveSystem system(registry, bus, &terrain);
    const world::TileCoord land = shore(terrain);
    const core::EntityId id = spawn_player(
        registry, {static_cast<float>(land.x) + 0.5f, 0.5f}, {1.0f, 0.0f});

    // When
    for (int step = 0; step < 20; ++step) system.update(0.1f);

    // Then
    EXPECT_LT(registry.get<world::Position>(id)->point.x, static_cast<float>(land.x + 1));
}

TEST(CharacterMoveSystem, PlayerBlockedDiagonallySlidesAlongTheBuilding) {
    // Given: a building to the right; the player walks right and down
    core::Registry registry;
    event::EventBus bus;
    const world::Terrain terrain(777);
    world::CharacterMoveSystem system(registry, bus, &terrain);
    place_building(registry, {1, 0});
    const core::EntityId id = spawn_player(registry, {0.74f, 0.5f}, {1.0f, 1.0f});

    // When
    system.update(0.1f);

    // Then: x keeps PlayerRadius from the building, y still advances
    const world::WorldPoint point = registry.get<world::Position>(id)->point;
    EXPECT_LE(point.x, 1.0f - world::PlayerRadius);
    EXPECT_GT(point.y, 0.5f);
}

TEST(CharacterMoveSystem, WorkerWalksIntoABuilding) {
    // Given: a non-player character heading into a building (workers go inside to work)
    core::Registry registry;
    event::EventBus bus;
    const world::Terrain terrain(777);
    world::CharacterMoveSystem system(registry, bus, &terrain);
    place_building(registry, {1, 0});
    const core::EntityId id = spawn_walker(registry, 1.0f, {1.0f, 0.0f});
    registry.get<world::Position>(id)->point = {0.5f, 0.5f};

    // When
    system.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(registry.get<world::Position>(id)->point.x, 1.5f);
}

TEST(CharacterMoveSystem, PlayerStuckInsideABuildingCanWalkOut) {
    // Given: a building placed over the player
    core::Registry registry;
    event::EventBus bus;
    const world::Terrain terrain(777);
    world::CharacterMoveSystem system(registry, bus, &terrain);
    place_building(registry, {0, 0});
    const core::EntityId id = spawn_player(registry, {0.5f, 0.5f}, {1.0f, 0.0f});

    // When
    system.update(0.5f);

    // Then
    EXPECT_FLOAT_EQ(registry.get<world::Position>(id)->point.x, 1.0f);
}

TEST(CharacterMoveSystem, BlockedPlayerStopsTouchingTheBuildingInsteadOfAStepShort) {
    // Given: a long step (fast player) that would end inside the building
    core::Registry registry;
    event::EventBus bus;
    const world::Terrain terrain(777);
    world::CharacterMoveSystem system(registry, bus, &terrain);
    place_building(registry, {1, 0});
    const core::EntityId id = spawn_player(registry, {0.2f, 0.5f}, {1.0f, 0.0f});

    // When
    system.update(1.0f);

    // Then: its body ends against the building's edge
    EXPECT_NEAR(registry.get<world::Position>(id)->point.x, 1.0f - world::PlayerRadius, 0.01f);
}
