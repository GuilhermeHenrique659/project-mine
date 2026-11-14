#include "graphics/raylib_texture_loader.hpp"

#include <raylib.h>

namespace graphics {

std::optional<Texture> RaylibTextureLoader::load(const std::string& path) {
    if (!FileExists(path.c_str())) {
        return std::nullopt;
    }
    const Texture2D loaded = LoadTexture(path.c_str());
    if (loaded.id == 0) {
        return std::nullopt;
    }
    return Texture{loaded.id, loaded.width, loaded.height, loaded.mipmaps, loaded.format};
}

namespace {
constexpr int PlaceholderSize = 32;
constexpr int PlaceholderCheckSize = 8;
} // namespace

// The classic "missing texture": a magenta and black checkerboard, generated in memory.
std::optional<Texture> RaylibTextureLoader::create_placeholder() {
    const Image image = GenImageChecked(PlaceholderSize, PlaceholderSize, PlaceholderCheckSize,
                                        PlaceholderCheckSize, MAGENTA, BLACK);
    const Texture2D loaded = LoadTextureFromImage(image);
    UnloadImage(image);
    if (loaded.id == 0) {
        return std::nullopt;
    }
    return Texture{loaded.id, loaded.width, loaded.height, loaded.mipmaps, loaded.format};
}

void RaylibTextureLoader::unload(const Texture& texture) {
    UnloadTexture({texture.id, texture.width, texture.height, texture.mipmaps, texture.format});
}

} // namespace graphics
