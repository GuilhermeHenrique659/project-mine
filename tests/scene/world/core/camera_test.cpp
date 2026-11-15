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

TEST(Camera, ZoomingInMakesTilesBigger) {
    // Given
    world::Camera camera;

    // When
    camera.zoom_by(1.0f);

    // Then
    EXPECT_GT(camera.zoom, 1.0f);
    const world::IsoProjection projection = camera.projection(1280.0f, 720.0f);
    EXPECT_FLOAT_EQ(projection.tile_width, 64.0f * camera.zoom);
    EXPECT_FLOAT_EQ(projection.tile_height, 32.0f * camera.zoom);
}

TEST(Camera, ZoomingOutMakesTilesSmaller) {
    // Given
    world::Camera camera;

    // When
    camera.zoom_by(-1.0f);

    // Then
    EXPECT_LT(camera.zoom, 1.0f);
}

TEST(Camera, ZoomIsLimitedInBothDirections) {
    // Given
    world::Camera camera;

    // When
    camera.zoom_by(1000.0f);
    const float zoomed_in = camera.zoom;
    camera.zoom_by(-1000.0f);

    // Then
    EXPECT_FLOAT_EQ(zoomed_in, world::Camera::MaxZoom);
    EXPECT_FLOAT_EQ(camera.zoom, world::Camera::MinZoom);
}

TEST(Camera, TheCenterStaysAtTheMiddleOfTheScreenWhenZoomed) {
    // Given
    world::Camera camera;
    camera.center = {7.5f, -3.0f};

    // When
    camera.zoom_by(2.0f);
    const world::ScreenPoint on_screen =
        camera.projection(1280.0f, 720.0f).world_to_screen(camera.center);

    // Then
    EXPECT_FLOAT_EQ(on_screen.x, 640.0f);
    EXPECT_FLOAT_EQ(on_screen.y, 360.0f);
}
