#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

#include "graphics/building_sprites.hpp"
#include "graphics/terrain_sprites.hpp"
#include "graphics/texture_catalog.hpp"
#include "graphics/texture_manager.hpp"
#include "graphics/unit_sprites.hpp"
#include "support/fake_texture_loader.hpp"

TEST(TextureCatalog, TerrainTexturesComeFromTheEnvironmentSpriteFolder) {
    // Given
    auto& textures = graphics::TextureManager::instance();
    textures.reset();
    auto loader = std::make_unique<FakeTextureLoader>();
    FakeTextureLoader& fake = *loader;
    textures.set_loader(std::move(loader));

    // When
    graphics::register_game_textures(textures, "data");
    for (const char* name : graphics::GrassTextures) {
        textures.find(name);
    }
    textures.find(graphics::WaterTexture);
    textures.find(graphics::CoalTexture);

    // Then
    const auto loaded = [&](const std::string& file) {
        const std::string path =
            (std::filesystem::path("data") / "sprite" / "environment" / file).string();
        return std::find(fake.loaded.begin(), fake.loaded.end(), path) != fake.loaded.end();
    };
    EXPECT_TRUE(loaded("grass_0.png"));
    EXPECT_TRUE(loaded("grass_3.png"));
    EXPECT_TRUE(loaded("water_sheet.png"));
    EXPECT_TRUE(loaded("coal.png"));
    textures.reset();
}

TEST(TextureCatalog, BuildingSheetsComeFromTheBuildingsSpriteFolder) {
    // Given
    auto& textures = graphics::TextureManager::instance();
    textures.reset();
    auto loader = std::make_unique<FakeTextureLoader>();
    FakeTextureLoader& fake = *loader;
    textures.set_loader(std::move(loader));

    // When
    graphics::register_game_textures(textures, "data");
    textures.find(graphics::HeadquartersTexture);

    // Then
    const std::string path =
        (std::filesystem::path("data") / "sprite" / "buildings" / "headquarters_sheet.png")
            .string();
    const std::vector<std::string> loaded = fake.loaded;
    textures.reset();
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded.front(), path);
}

TEST(TextureCatalog, TheCharacterSheetComesFromTheUnitsSpriteFolder) {
    // Given
    auto& textures = graphics::TextureManager::instance();
    textures.reset();
    auto loader = std::make_unique<FakeTextureLoader>();
    FakeTextureLoader& fake = *loader;
    textures.set_loader(std::move(loader));

    // When
    graphics::register_game_textures(textures, "data");
    textures.find(graphics::CharacterTexture);

    // Then
    const std::string path =
        (std::filesystem::path("data") / "sprite" / "units" / "character_sheet.png").string();
    const std::vector<std::string> loaded = fake.loaded;
    textures.reset();
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded.front(), path);
}
