#include <gtest/gtest.h>

#include "game/fixed_step.hpp"

TEST(FixedStep, AFrameRunsAsManyWholeTicksAsFitInIt) {
    // Given
    game::FixedStep clock(1.0f / 60.0f);

    // When
    const int ticks = clock.advance(0.5f);

    // Then
    EXPECT_EQ(ticks, 30);
}

TEST(FixedStep, TimeLeftOverFromAFrameCountsTowardsTheNextOne) {
    // Given
    game::FixedStep clock(0.1f);

    // When
    const int first = clock.advance(0.06f);
    const int second = clock.advance(0.06f);

    // Then
    EXPECT_EQ(first, 0);
    EXPECT_EQ(second, 1);
}

TEST(FixedStep, ManySmallFramesAddUpToTheSameTicksAsOneLongFrame) {
    // Given
    game::FixedStep small(1.0f / 60.0f);
    game::FixedStep large(1.0f / 60.0f);
    int small_ticks = 0;

    // When
    for (int frame = 0; frame < 144; ++frame) {
        small_ticks += small.advance(1.0f / 144.0f);
    }
    const int large_ticks = large.advance(1.0f);

    // Then
    EXPECT_EQ(small_ticks, 60);
    EXPECT_EQ(large_ticks, 60);
}

TEST(FixedStep, ALongFrameIsClampedSoTheSimulationDoesNotJump) {
    // Given / When
    const float hitch = game::clamp_frame_time(3.0f);
    const float normal = game::clamp_frame_time(0.02f);
    const float negative = game::clamp_frame_time(-1.0f);

    // Then
    EXPECT_FLOAT_EQ(hitch, game::MaxFrameTime);
    EXPECT_FLOAT_EQ(normal, 0.02f);
    EXPECT_FLOAT_EQ(negative, 0.0f);
}
