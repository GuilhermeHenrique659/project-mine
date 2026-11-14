#pragma once

#include <cstdint>

#include "scene/world/core/isometric.hpp"

namespace world {

enum class TileType { Grass, Water };

// Radius (in tiles) around world (0, 0) that is always grass, so the player never spawns
// in the water.
inline constexpr int SpawnClearRadius = 12;

// A flat world of grass and water, generated from a seed. Every tile is a pure function
// of the seed, so the same seed recreates the same world (a loaded save) and the world has
// no limits. Raylib-free.
class Terrain {
public:
    explicit Terrain(std::uint64_t seed) : seed_(seed) {}

    std::uint64_t seed() const { return seed_; }
    TileType tile_at(TileCoord tile) const;

private:
    std::uint64_t seed_;
};

} // namespace world
