#include "scene/world/core/terrain/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

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

constexpr float Pi = 3.14159265358979f;

// Starting nodes: one per type on a ring just outside the spawn clearing.
constexpr float StartNodeDistance = 24.0f;
constexpr float StartNodeRadius = 5.0f;
// Scattered nodes: one candidate per square cell, bigger the further from the spawn.
constexpr int NodeCellSize = 64;
constexpr float NodeChance = 0.6f;
constexpr float NodeGrowthPerTile = 0.02f;
constexpr float MaxNodeRadius = 22.0f;
// A node's edge is at radius * (ShapeMin + ShapeRange * noise): irregular, not a circle.
constexpr float ShapeMin = 0.7f;
constexpr float ShapeRange = 0.6f;
constexpr float ShapeMax = ShapeMin + ShapeRange;
constexpr float ShapeScale = 6.0f;

constexpr std::uint64_t ShapeSalt = 0x51ED270B0F1A3C5Dull;
constexpr std::uint64_t StartSalt = 0x2545F4914F6CDD1Dull;
constexpr std::uint64_t NodeSalt = 0x7F4A7C15D1E3B9A7ull;

// Each type also has one starting node near the spawn.
constexpr std::array<TileType, 4> OreTypes{TileType::IronOre, TileType::CopperOre, TileType::Coal,
                                           TileType::CrudeOil};

// Where a starting node may move to find dry land: anywhere around the spawn (its own angle
// tried first, then further and further from it) and out to MaxStartNodeDistance (small
// enough to stay close to the spawn).
constexpr int StartAngleSteps = 24;
constexpr float MaxStartNodeDistance = 36.0f;
constexpr int StartDistanceSteps = 4;
// Centres closer than this would make two starting nodes touch.
constexpr float MinStartNodeGap = 2.0f * StartNodeRadius * ShapeMax + 2.0f;

bool is_water(std::uint64_t seed, TileCoord tile) {
    const float value = noise(seed, static_cast<float>(tile.x) / NoiseScale,
                              static_cast<float>(tile.y) / NoiseScale);
    return value < WaterLevel;
}

// The node's footprint: the circle that holds all its tiles and their neighbours.
bool in_footprint(const OreNode& node, TileCoord tile) {
    const float reach = node.radius * ShapeMax + 1.5f;
    const float dx = static_cast<float>(tile.x) - node.x;
    const float dy = static_cast<float>(tile.y) - node.y;
    return dx * dx + dy * dy <= reach * reach;
}

// Water tiles in the node's footprint.
int water_under(const OreNode& node, std::uint64_t seed) {
    const int reach = static_cast<int>(std::ceil(node.radius * ShapeMax)) + 2;
    const int cx = static_cast<int>(std::round(node.x));
    const int cy = static_cast<int>(std::round(node.y));
    int water = 0;
    for (int y = cy - reach; y <= cy + reach; ++y) {
        for (int x = cx - reach; x <= cx + reach; ++x) {
            water += in_footprint(node, {x, y}) && is_water(seed, {x, y}) ? 1 : 0;
        }
    }
    return water;
}

bool overlaps(const OreNode& node, const OreNode* placed, std::size_t placed_count) {
    for (std::size_t i = 0; i < placed_count; ++i) {
        if (std::hypot(node.x - placed[i].x, node.y - placed[i].y) < MinStartNodeGap) {
            return true;
        }
    }
    return false;
}

// The starting node of `type` near `angle` on the ring: the first spot (closest to the ring
// position first) with no water around it and away from the nodes already placed, or the
// driest such spot when every one is wet.
OreNode place_starting_node(std::uint64_t seed, float angle, TileType type, const OreNode* placed,
                            std::size_t placed_count) {
    OreNode best{};
    int best_water = -1;
    for (int a = 0; a < StartAngleSteps; ++a) {
        // 0, +1, -1, +2, -2, ... steps away from the ring position
        const int step = (a + 1) / 2 * (a % 2 == 1 ? 1 : -1);
        const float shift = 2.0f * Pi * static_cast<float>(step) / StartAngleSteps;
        for (int d = 0; d < StartDistanceSteps; ++d) {
            const float distance = StartNodeDistance + (MaxStartNodeDistance - StartNodeDistance) *
                                                           static_cast<float>(d) / (StartDistanceSteps - 1);
            const OreNode node{std::cos(angle + shift) * distance, std::sin(angle + shift) * distance,
                               StartNodeRadius, type};
            if (overlaps(node, placed, placed_count)) {
                continue;
            }
            const int water = water_under(node, seed);
            if (water == 0) {
                return node;
            }
            if (best_water < 0 || water < best_water) {
                best = node;
                best_water = water;
            }
        }
    }
    return best;
}

bool node_contains(const OreNode& node, std::uint64_t seed, TileCoord tile) {
    const float dx = static_cast<float>(tile.x) - node.x;
    const float dy = static_cast<float>(tile.y) - node.y;
    const float shape = noise(seed ^ ShapeSalt, static_cast<float>(tile.x) / ShapeScale,
                              static_cast<float>(tile.y) / ShapeScale);
    const float edge = node.radius * (ShapeMin + ShapeRange * shape);
    return dx * dx + dy * dy < edge * edge;
}

// The scattered node of the cell holding `tile`, if the cell has one.
bool scattered_node(std::uint64_t seed, TileCoord tile, OreNode& node) {
    const auto cell = [](int value) {
        return static_cast<int>(std::floor(static_cast<float>(value) / static_cast<float>(NodeCellSize)));
    };
    const int cx = cell(tile.x);
    const int cy = cell(tile.y);
    const float centre_x = static_cast<float>(cx * NodeCellSize + NodeCellSize / 2);
    const float centre_y = static_cast<float>(cy * NodeCellSize + NodeCellSize / 2);
    const float distance = std::hypot(centre_x, centre_y);
    if (distance < static_cast<float>(StartingAreaRadius)) {
        return false;
    }
    const std::uint64_t bits = hash(seed ^ NodeSalt, cx, cy);
    const auto unit = [&](int shift) { return static_cast<float>((bits >> shift) & 0xFFFFu) / 65536.0f; };
    if (unit(0) >= NodeChance) {
        return false;
    }
    const float radius = std::min(MaxNodeRadius, StartNodeRadius + distance * NodeGrowthPerTile);
    // Keep the whole blob inside the cell, so only the tile's own cell needs checking.
    const float margin = radius * ShapeMax + 1.0f;
    const float room = static_cast<float>(NodeCellSize) - 2.0f * margin;
    node.x = static_cast<float>(cx * NodeCellSize) + margin + unit(16) * room;
    node.y = static_cast<float>(cy * NodeCellSize) + margin + unit(32) * room;
    node.radius = radius;
    node.type = OreTypes[static_cast<std::size_t>((bits >> 48) % OreTypes.size())];
    return true;
}

} // namespace

Terrain::Terrain(std::uint64_t seed) : seed_(seed) {
    const std::uint64_t bits = hash(seed ^ StartSalt, 0, 0);
    const float rotation = static_cast<float>(bits & 0xFFFFFFu) / 16777216.0f * 2.0f * Pi;
    for (std::size_t i = 0; i < starting_nodes_.size(); ++i) {
        const std::uint64_t jitter_bits = hash(seed ^ StartSalt, static_cast<int>(i) + 1, 0);
        const float jitter = (static_cast<float>(jitter_bits & 0xFFFFu) / 65536.0f - 0.5f) * 0.3f;
        // Iron, copper and coal sit a third of a turn apart; crude oil, added later, goes
        // between the first two, so older saves keep their other starting nodes in place.
        const float slot = i < 3 ? static_cast<float>(i) * 2.0f * Pi / 3.0f : Pi / 3.0f;
        const float angle = rotation + slot + jitter;
        starting_nodes_[i] = place_starting_node(seed, angle, OreTypes[i], starting_nodes_.data(), i);
    }
}

TileType Terrain::tile_at(TileCoord tile) const {
    if (std::abs(tile.x) <= SpawnClearRadius && std::abs(tile.y) <= SpawnClearRadius) {
        return TileType::Grass;
    }
    // Ore never replaces water: water first, then the nodes on what is left. A starting node
    // keeps its footprint dry (it only holds water when no dry spot was found near the spawn).
    const bool in_starting_footprint =
        std::any_of(starting_nodes_.begin(), starting_nodes_.end(),
                    [&](const OreNode& node) { return in_footprint(node, tile); });
    if (!in_starting_footprint && is_water(seed_, tile)) {
        return TileType::Water;
    }
    for (const OreNode& node : starting_nodes_) {
        if (node_contains(node, seed_, tile)) {
            return node.type;
        }
    }
    OreNode node;
    if (scattered_node(seed_, tile, node) && node_contains(node, seed_, tile)) {
        return node.type;
    }
    return TileType::Grass;
}

} // namespace world
