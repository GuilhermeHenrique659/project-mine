#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/systems/camera_follow_system.hpp"

TEST(CameraFollowSystem, CameraMovesToThePlayerControlledEntity) {
    // Given
    core::Registry registry;
    const core::EntityId other = registry.create("Other");
    registry.add(other, world::Position{{50.0f, 50.0f}});
    const core::EntityId player = registry.create("Player");
    registry.add(player, world::Position{{3.0f, -2.0f}});
    registry.add(player, world::PlayerControlled{});
    world::Camera camera;
    world::CameraFollowSystem system(registry, camera);

    // When
    system.update(0.016f);

    // Then
    EXPECT_FLOAT_EQ(camera.center.x, 3.0f);
    EXPECT_FLOAT_EQ(camera.center.y, -2.0f);
}

TEST(CameraFollowSystem, CameraKeepsFollowingAsThePlayerMoves) {
    // Given
    core::Registry registry;
    const core::EntityId player = registry.create("Player");
    registry.add(player, world::Position{{1.0f, 1.0f}});
    registry.add(player, world::PlayerControlled{});
    world::Camera camera;
    world::CameraFollowSystem system(registry, camera);
    system.update(0.016f);

    // When
    registry.get<world::Position>(player)->point = {8.0f, 4.0f};
    system.update(0.016f);

    // Then
    EXPECT_FLOAT_EQ(camera.center.x, 8.0f);
    EXPECT_FLOAT_EQ(camera.center.y, 4.0f);
}

TEST(CameraFollowSystem, CameraStaysPutWhenThereIsNoPlayer) {
    // Given
    core::Registry registry;
    const core::EntityId other = registry.create("Other");
    registry.add(other, world::Position{{9.0f, 9.0f}});
    world::Camera camera;
    camera.center = {2.0f, 2.0f};
    world::CameraFollowSystem system(registry, camera);

    // When
    system.update(0.016f);

    // Then
    EXPECT_FLOAT_EQ(camera.center.x, 2.0f);
    EXPECT_FLOAT_EQ(camera.center.y, 2.0f);
}
