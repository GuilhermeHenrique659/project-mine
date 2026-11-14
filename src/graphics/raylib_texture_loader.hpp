#pragma once

#include "graphics/texture_loader.hpp"

namespace graphics {

// Loads image files into GPU textures with raylib. Needs the window (GL context).
class RaylibTextureLoader : public TextureLoader {
public:
    std::optional<Texture> load(const std::string& path) override;
    std::optional<Texture> create_placeholder() override;
    void unload(const Texture& texture) override;
};

} // namespace graphics
