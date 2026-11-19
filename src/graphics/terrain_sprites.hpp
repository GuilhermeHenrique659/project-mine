#pragma once

#include "graphics/sprite.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace graphics {

// Terrain sprites (resource/sprite/environment/), 64x32 isometric diamonds.
inline constexpr int GrassVariants = 4;
inline constexpr int WaterFrames = 8;
inline constexpr float WaterFramesPerSecond = 8.0f;
inline constexpr int TerrainSpriteWidth = 64;
inline constexpr int TerrainSpriteHeight = 32;

// Texture names, registered by the game.
inline constexpr const char* GrassTextures[GrassVariants] = {"grass_0", "grass_1", "grass_2",
                                                             "grass_3"};
inline constexpr const char* WaterTexture = "water";
inline constexpr const char* IronOreTexture = "iron_ore";
inline constexpr const char* CopperOreTexture = "copper_ore";
inline constexpr const char* CoalTexture = "coal";

// The sprite of `tile` (of type `type`) after `seconds` of animation: grass picks a variant
// that is fixed per tile, water plays the frames of its sheet in a loop.
Sprite tile_sprite(::world::TileType type, ::world::TileCoord tile, float seconds);

} // namespace graphics
