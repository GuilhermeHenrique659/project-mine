#include <gtest/gtest.h>

#include <fstream>
#include <memory>

#include "graphics/icons.hpp"
#include "graphics/texture_manager.hpp"
#include "support/fake_texture_loader.hpp"
#include "support/temp_dir.hpp"

namespace {

class IconLoading : public testing::Test {
protected:
    void SetUp() override {
        graphics::TextureManager::instance().reset();
        graphics::TextureManager::instance().set_loader(std::make_unique<FakeTextureLoader>());
    }
    void TearDown() override { graphics::TextureManager::instance().reset(); }

    TempDir dir{"icons"};
};

} // namespace

TEST_F(IconLoading, EveryPngIsRegisteredUnderItsFileName) {
    // Given
    std::filesystem::create_directories(dir.path());
    std::ofstream(dir.path() / "icon_mine.png") << "png";
    std::ofstream(dir.path() / "notes.txt") << "text";
    auto& textures = graphics::TextureManager::instance();

    // When
    const int count = graphics::register_icons(textures, dir.path());

    // Then
    EXPECT_EQ(count, 1);
    EXPECT_NE(textures.find("icon_mine"), nullptr);
    EXPECT_EQ(textures.find("notes"), nullptr);
}

TEST_F(IconLoading, AMissingFolderRegistersNothing) {
    // Given
    auto& textures = graphics::TextureManager::instance();

    // When
    const int count = graphics::register_icons(textures, dir.path() / "nowhere");

    // Then
    EXPECT_EQ(count, 0);
}

TEST(IconNames, AnItemIconIsNamedAfterTheItemInSnakeCase) {
    // Given
    const auto type = ::world::ItemType::IronOre;

    // When
    const std::string name = graphics::item_icon_name(type);

    // Then
    EXPECT_EQ(name, "item_iron_ore");
}
