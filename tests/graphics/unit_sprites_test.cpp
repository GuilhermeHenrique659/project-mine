#include <gtest/gtest.h>

#include "graphics/unit_sprites.hpp"

using graphics::CharacterFrameSize;
using graphics::character_sprite;
using world::CharacterState;
using world::Direction;

namespace {

// The sheet cell a sprite shows: {column, row}.
std::pair<int, int> cell_of(const graphics::AnchoredSprite& sprite) {
    if (!sprite.sprite.source) {
        return {-1, -1};
    }
    return {static_cast<int>(sprite.sprite.source->x / CharacterFrameSize.width),
            static_cast<int>(sprite.sprite.source->y / CharacterFrameSize.height)};
}

} // namespace

TEST(UnitSprites, EachFacingHasItsOwnRowAndAStandingCharacterShowsTheFirstColumn) {
    // When
    const auto north = character_sprite(Direction::North, CharacterState::Idle, 3.0f);
    const auto south_west = character_sprite(Direction::SouthWest, CharacterState::Idle, 3.0f);

    // Then
    EXPECT_EQ(north.sprite.texture, graphics::CharacterTexture);
    EXPECT_EQ(cell_of(north), std::make_pair(0, 0));
    EXPECT_EQ(cell_of(south_west), std::make_pair(0, 5));
}

TEST(UnitSprites, WalkingPlaysTheWalkCycleInALoop) {
    // Given: one walk frame lasts 1 / 10 s, the cycle is 6 frames
    const float frame = 1.0f / graphics::CharacterWalkFramesPerSecond;

    // When
    const auto first = character_sprite(Direction::East, CharacterState::Moving, 0.5f * frame);
    const auto third = character_sprite(Direction::East, CharacterState::Moving, 2.5f * frame);
    const auto looped = character_sprite(Direction::East, CharacterState::Moving, 6.5f * frame);

    // Then: walk frames are columns 1..6 of the East row (2)
    EXPECT_EQ(cell_of(first), std::make_pair(1, 2));
    EXPECT_EQ(cell_of(third), std::make_pair(3, 2));
    EXPECT_EQ(cell_of(looped), std::make_pair(1, 2));
}

TEST(UnitSprites, RunningPlaysTheWalkCycleFaster) {
    // Given: a moment that is still the first walk frame
    const float seconds = 0.9f / graphics::CharacterWalkFramesPerSecond;

    // When
    const auto walking = character_sprite(Direction::South, CharacterState::Moving, seconds);
    const auto running = character_sprite(Direction::South, CharacterState::Running, seconds);

    // Then
    EXPECT_EQ(cell_of(walking), std::make_pair(1, 4));
    EXPECT_EQ(cell_of(running), std::make_pair(2, 4));
}
