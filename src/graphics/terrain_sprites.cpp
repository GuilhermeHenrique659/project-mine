#include "graphics/terrain_sprites.hpp"

#include <cstdint>

#include "graphics/animation.hpp"
#include "graphics/sprite_sheet.hpp"

namespace graphics {

namespace {

// A well-mixed hash of the tile, so grass variants do not form visible patterns.
int grass_variant(::world::TileCoord tile) {
    std::uint32_t h = static_cast<std::uint32_t>(tile.x) * 73856093u ^
                      static_cast<std::uint32_t>(tile.y) * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<int>(h % GrassVariants);
}

// The water sheet: WaterFrames cells in one row, played in a loop.
const Animation& water_animation() {
    static const Animation animation(
        SpriteSheet(WaterTexture, TerrainSpriteWidth, TerrainSpriteHeight, WaterFrames,
                    WaterFrames)
            .frames(0, WaterFrames),
        WaterFramesPerSecond, true);
    return animation;
}

} // namespace

Sprite tile_sprite(::world::TileType type, ::world::TileCoord tile, float seconds) {
    switch (type) {
    case ::world::TileType::Water: return *water_animation().sprite_at(seconds);
    case ::world::TileType::IronOre: return {IronOreTexture, std::nullopt};
    case ::world::TileType::CopperOre: return {CopperOreTexture, std::nullopt};
    case ::world::TileType::Coal: return {CoalTexture, std::nullopt};
    case ::world::TileType::Grass: break;
    }
    return {GrassTextures[grass_variant(tile)], std::nullopt};
}

} // namespace graphics
