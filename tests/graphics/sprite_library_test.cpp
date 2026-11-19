#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

#include "graphics/sprite_library.hpp"
#include "graphics/texture_manager.hpp"
#include "support/fake_texture_loader.hpp"
#include "support/temp_dir.hpp"

using graphics::SpriteClip;
using graphics::SpriteDefinition;
using graphics::SpriteLibrary;

namespace {

// A 64x32 sheet of 8 columns and 2 rows, anchored at (32, 30), with a looping clip over
// the whole row at 4 fps and a 3-frame clip that plays once.
SpriteDefinition sheet() {
    return {"sheet.png", 64, 32, 8, 2, {32.0f, 30.0f},
            {SpriteClip{"idle", 0, 8, 4.0f, true}, SpriteClip{"open", 2, 3, 4.0f, false}}};
}

void write(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path) << text;
}

class SpriteLibraryLoading : public testing::Test {
protected:
    void SetUp() override {
        graphics::TextureManager::instance().reset();
        auto fake = std::make_unique<FakeTextureLoader>();
        loader = fake.get();
        graphics::TextureManager::instance().set_loader(std::move(fake));
    }
    void TearDown() override { graphics::TextureManager::instance().reset(); }

    TempDir dir{"sprite_library"};
    FakeTextureLoader* loader = nullptr; // owned by the manager
};

} // namespace

TEST(SpriteLibrary, ALoopingClipPlaysAlongItsRowAndStartsOver) {
    // Given: a frame lasts 1/4 s
    SpriteLibrary library;
    const graphics::SpriteId id = library.add("thing", sheet(), graphics::TextureId{3});
    const graphics::ClipHandle idle = library.clip(id, "idle");

    // When
    const graphics::SpriteFrame third = library.frame(idle, 1, 2.5f * 0.25f);
    const graphics::SpriteFrame looped = library.frame(idle, 1, 9.5f * 0.25f);

    // Then: frames sit side by side, row 1 below row 0
    EXPECT_EQ(third.texture, graphics::TextureId{3});
    EXPECT_FLOAT_EQ(third.source.x, 2.0f * 64.0f);
    EXPECT_FLOAT_EQ(third.source.y, 32.0f);
    EXPECT_FLOAT_EQ(third.source.width, 64.0f);
    EXPECT_FLOAT_EQ(third.source.height, 32.0f);
    EXPECT_FLOAT_EQ(looped.source.x, 1.0f * 64.0f);
}

TEST(SpriteLibrary, AClipThatPlaysOnceHoldsItsLastFrame) {
    // Given
    SpriteLibrary library;
    const graphics::ClipHandle open =
        library.clip(library.add("door", sheet(), graphics::TextureId{}), "open");

    // When
    const graphics::SpriteFrame start = library.frame(open, 0, 0.0f);
    const graphics::SpriteFrame later = library.frame(open, 0, 100.0f);

    // Then: columns 2..4
    EXPECT_FLOAT_EQ(start.source.x, 2.0f * 64.0f);
    EXPECT_FLOAT_EQ(later.source.x, 4.0f * 64.0f);
}

TEST(SpriteLibrary, ARowOutsideTheSheetWrapsIntoIt) {
    // Given: 2 rows
    SpriteLibrary library;
    const graphics::ClipHandle idle =
        library.clip(library.add("thing", sheet(), graphics::TextureId{}), "idle");

    // When / Then
    EXPECT_FLOAT_EQ(library.frame(idle, 3, 0.0f).source.y, 32.0f);
    EXPECT_FLOAT_EQ(library.frame(idle, -1, 0.0f).source.y, 32.0f);
}

TEST(SpriteLibrary, AMissingClipFallsBackToIdleThenToTheFirstFrame) {
    // Given: one sheet with an idle clip, one with no clip at all
    SpriteLibrary library;
    const graphics::SpriteId with_idle = library.add("a", sheet(), graphics::TextureId{});
    SpriteDefinition bare = sheet();
    bare.clips.clear();
    const graphics::SpriteId without = library.add("b", bare, graphics::TextureId{});

    // When
    const graphics::SpriteFrame idle = library.frame(library.clip(with_idle, "run"), 0, 0.75f);
    const graphics::SpriteFrame first = library.frame(library.clip(without, "run"), 1, 0.75f);

    // Then: idle is on its 4th frame; the bare sheet stays on column 0
    EXPECT_FLOAT_EQ(idle.source.x, 3.0f * 64.0f);
    EXPECT_FLOAT_EQ(first.source.x, 0.0f);
    EXPECT_FLOAT_EQ(first.source.y, 32.0f);
}

TEST(SpriteLibrary, TheAnchorLandsOnTheGroundPointAtAnyZoom) {
    // Given
    SpriteLibrary library;
    const graphics::SpriteFrame frame =
        library.frame(library.clip(library.add("a", sheet(), graphics::TextureId{}), "idle"), 0,
                      0.0f);

    // When: drawn at half zoom with the ground point at (400, 300)
    const ui::Rect dest = frame.dest({400.0f, 300.0f}, 0.5f);

    // Then
    EXPECT_FLOAT_EQ(dest.width, 32.0f);
    EXPECT_FLOAT_EQ(dest.height, 16.0f);
    EXPECT_FLOAT_EQ(dest.x + 32.0f * 0.5f, 400.0f);
    EXPECT_FLOAT_EQ(dest.y + 30.0f * 0.5f, 300.0f);
}

TEST(SpriteLibrary, SpritesAreFoundByName) {
    // Given
    SpriteLibrary library;
    const graphics::SpriteId id = library.add("buildings/mine", sheet(), graphics::TextureId{});

    // When / Then
    EXPECT_EQ(library.find("buildings/mine"), id);
    EXPECT_FALSE(library.find("buildings/storage"));
}

TEST_F(SpriteLibraryLoading, EverySpriteFileIsNamedByItsPathAndItsTextureSitsBesideIt) {
    // Given
    write(dir.path() / "buildings" / "headquarters.sprite",
          "texture=headquarters_sheet.png\nframe_width=336\nframe_height=352\n");
    write(dir.path() / "units" / "character.sprite",
          "texture=character_sheet.png\nframe_width=32\nframe_height=48\nrows=8\n");
    write(dir.path() / "units" / "notes.txt", "not a sprite");

    // When
    const SpriteLibrary library =
        graphics::load_sprite_library(graphics::TextureManager::instance(), dir.path());

    // Then
    EXPECT_EQ(library.size(), 2u);
    const auto character = library.find("units/character");
    ASSERT_TRUE(character);
    EXPECT_EQ(library.rows(*character), 8);
    const graphics::SpriteFrame frame = library.frame(library.clip(*character, "idle"), 0, 0.0f);
    graphics::TextureManager::instance().find(frame.texture);
    ASSERT_EQ(loader->loaded.size(), 1u);
    EXPECT_EQ(loader->loaded.front(), (dir.path() / "units" / "character_sheet.png").string());
}

TEST_F(SpriteLibraryLoading, ABrokenSpriteFileIsSkippedAndTheOthersStillLoad) {
    // Given
    write(dir.path() / "environment" / "coal.sprite",
          "texture=coal.png\nframe_width=64\nframe_height=32\n");
    write(dir.path() / "environment" / "water.sprite", "texture=water_sheet.png\n");

    // When
    const SpriteLibrary library =
        graphics::load_sprite_library(graphics::TextureManager::instance(), dir.path());

    // Then
    EXPECT_TRUE(library.find("environment/coal"));
    EXPECT_FALSE(library.find("environment/water"));
}

TEST_F(SpriteLibraryLoading, AMissingSpriteFolderGivesAnEmptyLibrary) {
    // When
    const SpriteLibrary library = graphics::load_sprite_library(
        graphics::TextureManager::instance(), dir.path() / "nowhere");

    // Then
    EXPECT_EQ(library.size(), 0u);
}
