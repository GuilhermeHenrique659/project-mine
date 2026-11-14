#include <gtest/gtest.h>

#include "graphics/animation.hpp"
#include "graphics/sprite_sheet.hpp"

namespace {

// 3 columns of 16x32 frames, 5 frames: the second row has only two.
const graphics::SpriteSheet Sheet("hero", 16, 32, 3, 5);

} // namespace

TEST(SpriteSheet, FramesAreNumberedLeftToRightThenTopToBottom) {
    // Given / When
    const auto first = Sheet.frame(0);
    const auto fourth = Sheet.frame(3);

    // Then
    ASSERT_TRUE(first && fourth);
    EXPECT_EQ(first->texture, "hero");
    ASSERT_TRUE(first->source && fourth->source);
    EXPECT_FLOAT_EQ(first->source->x, 0.0f);
    EXPECT_FLOAT_EQ(first->source->y, 0.0f);
    EXPECT_FLOAT_EQ(fourth->source->x, 0.0f);
    EXPECT_FLOAT_EQ(fourth->source->y, 32.0f);
    EXPECT_FLOAT_EQ(fourth->source->width, 16.0f);
    EXPECT_FLOAT_EQ(fourth->source->height, 32.0f);
}

TEST(SpriteSheet, FrameOutsideTheSheetDoesNotExist) {
    // Given / When / Then
    EXPECT_FALSE(Sheet.frame(5).has_value());
    EXPECT_FALSE(Sheet.frame(-1).has_value());
}

TEST(SpriteSheet, FramesReturnsTheRunOfFramesThatExist) {
    // Given / When
    const auto run = Sheet.frames(3, 4); // asks for 3, 4, 5, 6 but the sheet ends at 4

    // Then
    ASSERT_EQ(run.size(), 2u);
    EXPECT_FLOAT_EQ(run[0].source->x, 0.0f);
    EXPECT_FLOAT_EQ(run[1].source->x, 16.0f);
}

namespace {

graphics::Animation walk(bool loop) {
    return graphics::Animation(Sheet.frames(0, 3), 10.0f, loop); // 0.1 s per frame
}

float frame_x(const graphics::Animation& animation, float seconds) {
    return animation.sprite_at(seconds)->source->x;
}

} // namespace

TEST(Animation, ShowsTheFrameThatMatchesTheElapsedTime) {
    // Given
    const auto animation = walk(true);

    // When / Then
    EXPECT_FLOAT_EQ(frame_x(animation, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(frame_x(animation, 0.15f), 16.0f);
    EXPECT_FLOAT_EQ(frame_x(animation, 0.25f), 32.0f);
}

TEST(Animation, LoopingAnimationStartsOverAfterTheLastFrame) {
    // Given
    const auto animation = walk(true);

    // When / Then: 0.3 s is the end of the cycle
    EXPECT_FLOAT_EQ(frame_x(animation, 0.35f), 0.0f);
}

TEST(Animation, NonLoopingAnimationStaysOnTheLastFrame) {
    // Given
    const auto animation = walk(false);

    // When / Then
    EXPECT_FLOAT_EQ(frame_x(animation, 5.0f), 32.0f);
}

TEST(Animation, NegativeTimeShowsTheFirstFrameAndNoFramesShowNothing) {
    // Given
    const auto animation = walk(true);
    const graphics::Animation empty({}, 10.0f, true);

    // When / Then
    EXPECT_FLOAT_EQ(frame_x(animation, -1.0f), 0.0f);
    EXPECT_EQ(empty.sprite_at(0.0f), nullptr);
}
