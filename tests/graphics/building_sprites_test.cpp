#include <gtest/gtest.h>

#include "graphics/building_sprites.hpp"

using world::BuildingType;

TEST(BuildingSprites, TheHeadquartersAntennaTurnsAndStartsOverAfterAFullTurn) {
    // Given: one frame lasts 1 / 4 s, a full turn is 8 frames (2 s)
    const float frame = 1.0f / graphics::HeadquartersFramesPerSecond;

    // When
    const auto start = graphics::building_sprite(BuildingType::Headquarters, 0.0f);
    const auto third = graphics::building_sprite(BuildingType::Headquarters, 2.5f * frame);
    const auto turned = graphics::building_sprite(BuildingType::Headquarters, 8.5f * frame);

    // Then: frames sit side by side in the sheet
    ASSERT_TRUE(start && third && turned);
    EXPECT_EQ(start->sprite.texture, graphics::HeadquartersTexture);
    ASSERT_TRUE(third->sprite.source && turned->sprite.source);
    EXPECT_FLOAT_EQ(third->sprite.source->x, 2.0f * graphics::HeadquartersFrameSize.width);
    EXPECT_FLOAT_EQ(third->sprite.source->height, graphics::HeadquartersFrameSize.height);
    EXPECT_FLOAT_EQ(turned->sprite.source->x, 0.0f);
}

TEST(BuildingSprites, BuildingsWithoutASpriteHaveNone) {
    // When / Then
    EXPECT_FALSE(graphics::building_sprite(BuildingType::Mine, 0.0f).has_value());
}

TEST(BuildingSprites, TheAnchorLandsOnTheGroundPointAtAnyZoom) {
    // Given
    const auto hq = graphics::building_sprite(BuildingType::Headquarters, 0.0f);
    ASSERT_TRUE(hq);

    // When: drawn at half zoom with the footprint centre at (400, 300)
    const ui::Rect dest = hq->dest({400.0f, 300.0f}, 0.5f);

    // Then
    EXPECT_FLOAT_EQ(dest.width, graphics::HeadquartersFrameSize.width * 0.5f);
    EXPECT_FLOAT_EQ(dest.height, graphics::HeadquartersFrameSize.height * 0.5f);
    EXPECT_FLOAT_EQ(dest.x + graphics::HeadquartersAnchor.x * 0.5f, 400.0f);
    EXPECT_FLOAT_EQ(dest.y + graphics::HeadquartersAnchor.y * 0.5f, 300.0f);
}
