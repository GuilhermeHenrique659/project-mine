#include "graphics/texture_catalog.hpp"

#include <string>

#include "graphics/terrain_sprites.hpp"

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
}

} // namespace graphics
