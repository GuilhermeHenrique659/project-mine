#pragma once

#include <filesystem>
#include <string>

#include "graphics/texture_manager.hpp"
#include "scene/world/core/items/item.hpp"

namespace graphics {

// UI icons are plain PNGs under <data dir>/icon/ (made by tools/sprites/icons.py), each
// registered as a texture named by its file name without extension: "icon_mine.png" is
// drawn by graphics::draw_sprite(canvas, {"icon_mine"}, ...). Naming convention:
//   icon_<building key in snake_case>   construction menu entries (see construction.cpp)
//   icon_tab_<category>                 construction menu tabs
//   item_<item name in snake_case>      inventory slots (item_icon_name())
//   menu_<button>                       title menu buttons
// Returns how many were registered; a missing folder registers none.
int register_icons(TextureManager& textures, const std::filesystem::path& folder);

// "item_iron_ore" for ItemType::IronOre.
std::string item_icon_name(::world::ItemType type);

} // namespace graphics
