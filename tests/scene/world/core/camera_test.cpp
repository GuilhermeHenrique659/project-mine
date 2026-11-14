#include <gtest/gtest.h>

#include "scene/world/core/camera.hpp"

TEST(Camera, ItsCenterIsDrawnAtTheMiddleOfTheScreen) {
    // Given
    world::Camera camera;
    camera.center = {7.5f, -3.0f};

    // When
    const world::IsoProjection projection = camera.projection(1280.0f, 720.0f);

    // Then
    const world::ScreenPoint on_screen = projection.world_to_screen(camera.center);
    EXPECT_FLOAT_EQ(on_screen.x, 640.0f);
    EXPECT_FLOAT_EQ(on_screen.y, 360.0f);
}

TEST(Camera, PointsAreDrawnRelativeToTheCenter) {
    // Given
    world::Camera camera;
    camera.center = {4.0f, 9.0f};

    // When
    const world::IsoProjection projection = camera.projection(1280.0f, 720.0f);

    // Then: one tile along world +x is (+32, +16) pixels from the middle of the screen
    const world::ScreenPoint on_screen = projection.world_to_screen({5.0f, 9.0f});
    EXPECT_FLOAT_EQ(on_screen.x, 640.0f + 32.0f);
    EXPECT_FLOAT_EQ(on_screen.y, 360.0f + 16.0f);
}
