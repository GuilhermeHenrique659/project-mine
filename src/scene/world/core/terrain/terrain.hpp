#pragma once

#include <array>
#include <cstdint>

#include "scene/world/core/isometric.hpp"

namespace world {

enum class TileType { Grass, Water, IronOre, CopperOre, Coal };

inline constexpr bool is_ore(TileType type) {
    return type == TileType::IronOre || type == TileType::CopperOre || type == TileType::Coal;
}

// Radius (in tiles) around world (0, 0) that is always grass, so the player never spawns
// in the water or on ore.
inline constexpr int SpawnClearRadius = 12;

// Radius (in tiles) around world (0, 0) with no scattered ore nodes: only the three
// starting nodes (iron, copper, coal) are there.
inline constexpr int StartingAreaRadius = 64;

// An ore node: a blob of tiles of one type around a centre.
struct OreNode {
    float x = 0.0f;
    float y = 0.0f;
    float radius = 0.0f;
    TileType type = TileType::IronOre;
};

// A flat world of grass, water and ore nodes (iron, copper, coal), generated from a seed.
// Three medium starting nodes (one per type) lie on dry land just outside the spawn
// clearing; further away scattered nodes get bigger with the distance from the spawn. Ore is
// never placed on water. Every tile is a pure function of the seed, so the same seed recreates the same
// world (a loaded save) and the world has no limits. Raylib-free.
class Terrain {
public:
    explicit Terrain(std::uint64_t seed);

    std::uint64_t seed() const { return seed_; }
    TileType tile_at(TileCoord tile) const;

private:
    std::uint64_t seed_;
    std::array<OreNode, 3> starting_nodes_;
};

} // namespace world
