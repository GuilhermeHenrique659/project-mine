#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "graphics/texture_manager.hpp"
#include "support/fake_texture_loader.hpp"

namespace {

class TextureManagerTest : public testing::Test {
protected:
    void SetUp() override {
        graphics::TextureManager::instance().reset();
        auto fake = std::make_unique<FakeTextureLoader>();
        loader = fake.get();
        graphics::TextureManager::instance().set_loader(std::move(fake));
    }
    void TearDown() override { graphics::TextureManager::instance().reset(); }

    graphics::TextureManager& manager() { return graphics::TextureManager::instance(); }

    FakeTextureLoader* loader = nullptr; // owned by the manager
};

} // namespace

TEST_F(TextureManagerTest, RegisteredTextureIsLoadedFromItsFileOnFirstUse) {
    // Given
    manager().register_texture("player", "player.png");

    // When
    const graphics::Texture* texture = manager().get("player");

    // Then
    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(texture->width, 32);
    ASSERT_EQ(loader->loaded.size(), 1u);
    EXPECT_EQ(loader->loaded.front(), "player.png");
}

TEST_F(TextureManagerTest, TextureIsLoadedOnlyOnceNoMatterHowOftenItIsAsked) {
    // Given
    manager().register_texture("player", "player.png");

    // When
    const graphics::Texture* first = manager().get("player");
    const graphics::Texture* second = manager().get("player");

    // Then
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first->id, second->id);
    EXPECT_EQ(loader->loaded.size(), 1u);
}

TEST_F(TextureManagerTest, UnregisteredNameGivesTheFallbackTexture) {
    // Given
    manager().register_texture("player", "player.png");

    // When
    const graphics::Texture* texture = manager().get("tree");

    // Then
    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(texture->id, FakeTextureLoader::PlaceholderId);
    EXPECT_TRUE(loader->loaded.empty());
}

TEST_F(TextureManagerTest, FileThatCannotBeLoadedGivesTheFallbackAndIsNotRetried) {
    // Given
    manager().register_texture("ghost", "missing.png");

    // When
    const graphics::Texture* first = manager().get("ghost");
    const graphics::Texture* second = manager().get("ghost");

    // Then
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->id, FakeTextureLoader::PlaceholderId);
    EXPECT_EQ(second, first);
    EXPECT_EQ(loader->loaded.size(), 1u);
}

TEST_F(TextureManagerTest, FindNeverGivesTheFallback) {
    // Given
    manager().register_texture("ghost", "missing.png");

    // When / Then
    EXPECT_EQ(manager().find("tree"), nullptr);
    EXPECT_EQ(manager().find("ghost"), nullptr);
}

TEST_F(TextureManagerTest, FallbackIsCreatedOnlyOnceAndSharedByEveryMissingTexture) {
    // Given / When
    const graphics::Texture* first = manager().get("tree");
    const graphics::Texture* second = manager().get("rock");

    // Then
    EXPECT_EQ(first, second);
    EXPECT_EQ(loader->placeholders_created, 1);
}

TEST_F(TextureManagerTest, UnloadAllReleasesLoadedTexturesAndTheyLoadAgainOnNextUse) {
    // Given
    manager().register_texture("player", "player.png");
    manager().register_texture("tree", "tree.png"); // never asked for, so never loaded
    const std::uint32_t first_id = manager().get("player")->id;

    // When
    manager().unload_all();
    const graphics::Texture* reloaded = manager().get("player");

    // Then
    ASSERT_EQ(loader->unloaded.size(), 1u);
    EXPECT_EQ(loader->unloaded.front(), first_id);
    ASSERT_NE(reloaded, nullptr);
    EXPECT_EQ(loader->loaded.size(), 2u);
}

TEST_F(TextureManagerTest, UnloadAllAlsoReleasesTheFallbackAndItIsCreatedAgainWhenNeeded) {
    // Given
    manager().get("tree");

    // When
    manager().unload_all();
    const graphics::Texture* again = manager().get("tree");

    // Then
    ASSERT_EQ(loader->unloaded.size(), 1u);
    EXPECT_EQ(loader->unloaded.front(), FakeTextureLoader::PlaceholderId);
    ASSERT_NE(again, nullptr);
    EXPECT_EQ(loader->placeholders_created, 2);
}

TEST_F(TextureManagerTest, MissingTextureGivesNothingWhenTheFallbackCannotBeCreated) {
    // Given
    loader->placeholder_works = false;

    // When
    const graphics::Texture* texture = manager().get("tree");

    // Then
    EXPECT_EQ(texture, nullptr);
}

TEST_F(TextureManagerTest, WithoutALoaderNothingIsLoadedNotEvenTheFallback) {
    // Given
    manager().reset();
    manager().register_texture("player", "player.png");

    // When / Then
    EXPECT_EQ(manager().get("player"), nullptr);
    EXPECT_EQ(manager().get("tree"), nullptr);
}

TEST_F(TextureManagerTest, ATextureFoundByItsIdIsTheOneOfItsNameAndLoadsOnce) {
    // Given
    const graphics::TextureId id = manager().register_texture("player", "player.png");

    // When
    const graphics::Texture* by_id = manager().find(id);
    const graphics::Texture* by_name = manager().find("player");

    // Then
    ASSERT_NE(by_id, nullptr);
    EXPECT_EQ(by_id, by_name);
    EXPECT_EQ(loader->loaded.size(), 1u);
}

TEST_F(TextureManagerTest, RegisteringANameAgainKeepsItsIdAndLoadsTheNewFile) {
    // Given
    const graphics::TextureId first = manager().register_texture("player", "old.png");
    manager().find(first);

    // When
    const graphics::TextureId second = manager().register_texture("player", "new.png");
    manager().find(second);

    // Then
    EXPECT_EQ(first, second);
    ASSERT_EQ(loader->loaded.size(), 2u);
    EXPECT_EQ(loader->loaded.back(), "new.png");
}
