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
