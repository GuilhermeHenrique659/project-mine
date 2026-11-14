#pragma once

#include <cstdint>

namespace graphics {

// Handle of a texture living on the GPU. Raylib-free: only raylib_texture_loader.cpp
// converts it to and from the raylib type.
struct Texture {
    std::uint32_t id = 0;
    int width = 0;
    int height = 0;
    int mipmaps = 1;
    int format = 0;
};

} // namespace graphics
