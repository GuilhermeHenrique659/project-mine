#include "scene/world/core/terrain/terrain.hpp"

#include <cmath>

namespace world {

namespace {

// Size, in tiles, of one noise cell: the scale of the lakes.
constexpr float NoiseScale = 40.0f;
// Noise values below this are water (noise is in [0, 1)).
constexpr float WaterLevel = 0.38f;

// SplitMix64 finalizer: a well mixed hash of the seed and a lattice point.
std::uint64_t hash(std::uint64_t seed, int x, int y) {
    std::uint64_t value = seed;
    value += 0x9E3779B97F4A7C15ull * (static_cast<std::uint64_t>(static_cast<std::int64_t>(x)) + 1);
    value ^= value >> 30;
    value *= 0xBF58476D1CE4E5B9ull;
    value += 0x94D049BB133111EBull * (static_cast<std::uint64_t>(static_cast<std::int64_t>(y)) + 1);
    value ^= value >> 27;
    value *= 0x94D049BB133111EBull;
    value ^= value >> 31;
    return value;
}

float lattice_value(std::uint64_t seed, int x, int y) {
    return static_cast<float>(hash(seed, x, y) >> 40) / static_cast<float>(1u << 24);
}

float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

// Value noise in [0, 1): random values on a grid, blended smoothly in between.
float noise(std::uint64_t seed, float x, float y) {
    const float floor_x = std::floor(x);
    const float floor_y = std::floor(y);
    const int x0 = static_cast<int>(floor_x);
    const int y0 = static_cast<int>(floor_y);
    const float tx = smooth(x - floor_x);
    const float ty = smooth(y - floor_y);
    const float top = lattice_value(seed, x0, y0) * (1.0f - tx) + lattice_value(seed, x0 + 1, y0) * tx;
    const float bottom =
        lattice_value(seed, x0, y0 + 1) * (1.0f - tx) + lattice_value(seed, x0 + 1, y0 + 1) * tx;
    return top * (1.0f - ty) + bottom * ty;
}

} // namespace

TileType Terrain::tile_at(TileCoord tile) const {
    if (std::abs(tile.x) <= SpawnClearRadius && std::abs(tile.y) <= SpawnClearRadius) {
        return TileType::Grass;
    }
    const float value = noise(seed_, static_cast<float>(tile.x) / NoiseScale,
                              static_cast<float>(tile.y) / NoiseScale);
    return value < WaterLevel ? TileType::Water : TileType::Grass;
}

} // namespace world
