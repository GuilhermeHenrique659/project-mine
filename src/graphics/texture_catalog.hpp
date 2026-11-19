#pragma once

#include <filesystem>

#include "graphics/texture_manager.hpp"

namespace graphics {

// Registers every texture the game draws, by name, with its file under
// <data_dir>/sprite/ (terrain in sprite/environment/). Files are not opened here.
void register_game_textures(TextureManager& textures, const std::filesystem::path& data_dir);

} // namespace graphics
