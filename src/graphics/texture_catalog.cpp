#include "graphics/texture_catalog.hpp"

#include <string>

#include "graphics/building_sprites.hpp"
#include "graphics/terrain_sprites.hpp"
#include "graphics/unit_sprites.hpp"

namespace graphics {

void register_game_textures(TextureManager& textures, const std::filesystem::path& data_dir) {
    const std::filesystem::path environment = data_dir / "sprite" / "environment";
    const auto add = [&](const char* name, const std::string& file) {
        textures.register_texture(name, (environment / file).string());
    };
    for (const char* name : GrassTextures) {
        add(name, std::string(name) + ".png");
    }
    add(WaterTexture, "water_sheet.png");
    add(IronOreTexture, "iron_ore.png");
    add(CopperOreTexture, "copper_ore.png");
    add(CoalTexture, "coal.png");

    const std::filesystem::path buildings = data_dir / "sprite" / "buildings";
    textures.register_texture(HeadquartersTexture,
                              (buildings / "headquarters_sheet.png").string());

    const std::filesystem::path units = data_dir / "sprite" / "units";
    textures.register_texture(CharacterTexture, (units / "character_sheet.png").string());
}

} // namespace graphics
