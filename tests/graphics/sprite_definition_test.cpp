#include <gtest/gtest.h>

#include <string>

#include "graphics/sprite_definition.hpp"

using graphics::parse_sprite_definition;

TEST(SpriteDefinition, ReadsTheSheetLayoutTheAnchorAndTheClips) {
    // Given
    const std::string text = "# the character\n"
                             "texture=character_sheet.png\n"
                             "frame_width=32\n"
                             "frame_height=48\n"
                             "columns=7\n"
                             "rows=8\n"
                             "anchor_x=16\n"
                             "anchor_y=44.5\n"
                             "\n"
                             "clip.idle=0,1,0,loop\n"
                             "clip.walk=1,6,10,once\n";

    // When
    const auto definition = parse_sprite_definition(text);

    // Then
    ASSERT_TRUE(definition);
    EXPECT_EQ(definition->texture, "character_sheet.png");
    EXPECT_EQ(definition->frame_width, 32);
    EXPECT_EQ(definition->frame_height, 48);
    EXPECT_EQ(definition->columns, 7);
    EXPECT_EQ(definition->rows, 8);
    EXPECT_FLOAT_EQ(definition->anchor.x, 16.0f);
    EXPECT_FLOAT_EQ(definition->anchor.y, 44.5f);
    ASSERT_EQ(definition->clips.size(), 2u);
    const graphics::SpriteClip& walk = definition->clips[1];
    EXPECT_EQ(walk.name, "walk");
    EXPECT_EQ(walk.first, 1);
    EXPECT_EQ(walk.count, 6);
    EXPECT_FLOAT_EQ(walk.frames_per_second, 10.0f);
    EXPECT_FALSE(walk.loop);
}

TEST(SpriteDefinition, ASingleImageNeedsOnlyItsTextureAndSize) {
    // When
    const auto definition =
        parse_sprite_definition("texture=coal.png\nframe_width=64\nframe_height=32\n");

    // Then: one frame, anchored on its top-left corner, no clips
    ASSERT_TRUE(definition);
    EXPECT_EQ(definition->columns, 1);
    EXPECT_EQ(definition->rows, 1);
    EXPECT_FLOAT_EQ(definition->anchor.x, 0.0f);
    EXPECT_TRUE(definition->clips.empty());
}

TEST(SpriteDefinition, WindowsLineEndingsAndSpacesAroundValuesAreAccepted) {
    // When
    const auto definition =
        parse_sprite_definition("texture = coal.png\r\nframe_width= 64\r\nframe_height =32\r\n");

    // Then
    ASSERT_TRUE(definition);
    EXPECT_EQ(definition->texture, "coal.png");
    EXPECT_EQ(definition->frame_width, 64);
}

TEST(SpriteDefinition, MissingTextureOrFrameSizeIsRejected) {
    // When / Then
    std::string error;
    EXPECT_FALSE(parse_sprite_definition("frame_width=64\nframe_height=32\n", &error));
    EXPECT_NE(error.find("texture"), std::string::npos);
    EXPECT_FALSE(parse_sprite_definition("texture=a.png\nframe_height=32\n"));
    EXPECT_FALSE(parse_sprite_definition("texture=a.png\nframe_width=0\nframe_height=32\n"));
}

TEST(SpriteDefinition, AnUnknownKeyIsRejectedSoTyposDoNotGoUnnoticed) {
    // When
    std::string error;
    const auto definition = parse_sprite_definition(
        "texture=a.png\nframe_width=64\nframe_height=32\nanchr_x=3\n", &error);

    // Then
    EXPECT_FALSE(definition);
    EXPECT_NE(error.find("anchr_x"), std::string::npos);
}

TEST(SpriteDefinition, AnInvalidNumberIsRejected) {
    // When / Then
    EXPECT_FALSE(parse_sprite_definition("texture=a.png\nframe_width=6x\nframe_height=32\n"));
    EXPECT_FALSE(
        parse_sprite_definition("texture=a.png\nframe_width=64\nframe_height=32\nrows=-1\n"));
}

TEST(SpriteDefinition, AClipRunningOutsideItsRowIsRejected) {
    // Given: 8 columns
    const std::string sheet = "texture=a.png\nframe_width=64\nframe_height=32\ncolumns=8\n";

    // When / Then: the last column is 7
    EXPECT_TRUE(parse_sprite_definition(sheet + "clip.idle=0,8,4,loop\n"));
    EXPECT_FALSE(parse_sprite_definition(sheet + "clip.idle=1,8,4,loop\n"));
    EXPECT_FALSE(parse_sprite_definition(sheet + "clip.idle=0,0,4,loop\n"));
    EXPECT_FALSE(parse_sprite_definition(sheet + "clip.idle=0,2,4,forever\n"));
    EXPECT_FALSE(parse_sprite_definition(sheet + "clip.idle=0,2,4\n"));
}
