#pragma once

#include <optional>
#include <string>

#include "graphics/texture.hpp"

namespace graphics {

// Where textures come from. The real one reads image files into the GPU; tests use a fake.
class TextureLoader {
public:
    virtual ~TextureLoader() = default;
    // Returns nothing when the file cannot be loaded.
    virtual std::optional<Texture> load(const std::string& path) = 0;
    // A generic picture (a checkerboard) for when a texture is missing; needs no file.
    virtual std::optional<Texture> create_placeholder() = 0;
    virtual void unload(const Texture& texture) = 0;
};

} // namespace graphics
