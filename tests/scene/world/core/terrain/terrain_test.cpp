#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include "scene/world/core/terrain/terrain.hpp"

namespace {

constexpr int Extent = 150;

int count_tiles(const world::Terrain& terrain, world::TileType type) {
    int count = 0;
    for (int y = -Extent; y <= Extent; ++y) {
        for (int x = -Extent; x <= Extent; ++x) {
            count += terrain.tile_at({x, y}) == type ? 1 : 0;
        }
    }
    return count;
}

bool same_area(const world::Terrain& a, const world::Terrain& b) {
    for (int y = -Extent; y <= Extent; ++y) {
        for (int x = -Extent; x <= Extent; ++x) {
            if (a.tile_at({x, y}) != b.tile_at({x, y})) {
                return false;
            }
        }
    }
    return true;
}

// 0, 1, 2, 3 for iron, copper, coal, crude oil.
std::size_t ore_index(world::TileType type) {
    return static_cast<std::size_t>(type) - static_cast<std::size_t>(world::TileType::IronOre);
}

struct Node {
    world::TileType type;
    int size;
    bool touches_border;
};

// Connected groups (4-neighbour) of ore tiles of one type inside [x0, x0+w) x [y0, y0+h).
std::vector<Node> find_nodes(const world::Terrain& terrain, int x0, int y0, int w, int h) {
    std::vector<Node> nodes;
    std::vector<bool> seen(static_cast<std::size_t>(w) * h, false);
    const auto index = [&](int x, int y) { return static_cast<std::size_t>(y - y0) * w + (x - x0); };
    for (int y = y0; y < y0 + h; ++y) {
        for (int x = x0; x < x0 + w; ++x) {
            const world::TileType type = terrain.tile_at({x, y});
            if (!world::is_ore(type) || seen[index(x, y)]) {
                continue;
            }
            Node node{type, 0, false};
            std::vector<world::TileCoord> pending{{x, y}};
            seen[index(x, y)] = true;
            while (!pending.empty()) {
                const world::TileCoord tile = pending.back();
                pending.pop_back();
                ++node.size;
                if (tile.x == x0 || tile.y == y0 || tile.x == x0 + w - 1 || tile.y == y0 + h - 1) {
                    node.touches_border = true;
                }
                const world::TileCoord next[] = {
                    {tile.x + 1, tile.y}, {tile.x - 1, tile.y}, {tile.x, tile.y + 1}, {tile.x, tile.y - 1}};
                for (const world::TileCoord n : next) {
                    if (n.x < x0 || n.y < y0 || n.x >= x0 + w || n.y >= y0 + h || seen[index(n.x, n.y)] ||
                        terrain.tile_at(n) != type) {
                        continue;
                    }
                    seen[index(n.x, n.y)] = true;
                    pending.push_back(n);
                }
            }
            nodes.push_back(node);
        }
    }
    return nodes;
}

// Average size of the nodes fully inside the window (those not cut by its border).
double average_node_size(int x0, int y0, int w, int h, int seeds) {
    double total = 0.0;
    int count = 0;
    for (int seed = 0; seed < seeds; ++seed) {
        const world::Terrain terrain(static_cast<std::uint64_t>(seed));
        for (const Node& node : find_nodes(terrain, x0, y0, w, h)) {
            if (node.touches_border) {
                continue;
            }
            total += node.size;
            ++count;
        }
    }
    return count == 0 ? 0.0 : total / count;
}

} // namespace

TEST(Terrain, SameSeedRecreatesTheSameWorld) {
    // Given
    const world::Terrain created(12345);

    // When
    const world::Terrain loaded(12345);

    // Then
    EXPECT_TRUE(same_area(created, loaded));
}

TEST(Terrain, DifferentSeedsMakeDifferentWorlds) {
    // Given / When
    const world::Terrain first(1);
    const world::Terrain second(2);

    // Then
    EXPECT_FALSE(same_area(first, second));
}

TEST(Terrain, WorldHasBothGrassAndWater) {
    // Given / When
    const world::Terrain terrain(777);

    // Then
    EXPECT_GT(count_tiles(terrain, world::TileType::Grass), 0);
    EXPECT_GT(count_tiles(terrain, world::TileType::Water), 0);
}

TEST(Terrain, SpawnAreaIsAlwaysGrass) {
    // Given
    for (std::uint64_t seed = 0; seed < 50; ++seed) {
        const world::Terrain terrain(seed);

        // When / Then
        for (int y = -world::SpawnClearRadius; y <= world::SpawnClearRadius; ++y) {
            for (int x = -world::SpawnClearRadius; x <= world::SpawnClearRadius; ++x) {
                ASSERT_EQ(terrain.tile_at({x, y}), world::TileType::Grass)
                    << "seed " << seed << " tile " << x << "," << y;
            }
        }
    }
}

TEST(Terrain, LandAndWaterAreasAreNeitherTinyNorHugeComparedToThePlayer) {
    // Given: the player is about one tile wide, so areas of a few tiles make it a giant,
    // while areas of hundreds of tiles make the world look empty
    constexpr int RowLength = 600;
    constexpr int Rows = 4;
    constexpr int Seeds = 40;
    constexpr double MinAverageRun = 50.0;
    constexpr double MaxAverageRun = 120.0;

    // When: walking along rows away from the spawn clearing, counting runs of land (grass or
    // ore) and of water
    const auto is_water = [](world::TileType type) { return type == world::TileType::Water; };
    int runs = 0;
    for (std::uint64_t seed = 0; seed < Seeds; ++seed) {
        const world::Terrain terrain(seed);
        for (int row = 40; row < 40 + Rows * 5; row += 5) {
            bool previous = is_water(terrain.tile_at({-RowLength / 2, row}));
            ++runs;
            for (int x = -RowLength / 2 + 1; x < RowLength / 2; ++x) {
                const bool water = is_water(terrain.tile_at({x, row}));
                runs += water != previous ? 1 : 0;
                previous = water;
            }
        }
    }

    // Then: judged over many seeds, since one seed only has a few runs
    const double average_run = static_cast<double>(Seeds) * Rows * RowLength / runs;
    EXPECT_GE(average_run, MinAverageRun);
    EXPECT_LE(average_run, MaxAverageRun);
}

TEST(Terrain, EachOreHasAMediumNodeCloseToTheSpawn) {
    // Given
    for (std::uint64_t seed = 0; seed < 40; ++seed) {
        const world::Terrain terrain(seed);

        // When: looking at the area around the spawn
        const std::vector<Node> nodes = find_nodes(terrain, -45, -45, 91, 91);

        // Then: exactly one node of each type, of medium size
        std::array<int, 3> per_type{};
        for (const Node& node : nodes) {
            ++per_type[ore_index(node.type)];
            EXPECT_GE(node.size, 40) << "seed " << seed;
            EXPECT_LE(node.size, 150) << "seed " << seed;
        }
        EXPECT_EQ(per_type[0], 1) << "seed " << seed;
        EXPECT_EQ(per_type[1], 1) << "seed " << seed;
        EXPECT_EQ(per_type[2], 1) << "seed " << seed;
    }
}

TEST(Terrain, StartingOreIsOutsideTheSpawnClearing) {
    // Given
    for (std::uint64_t seed = 0; seed < 40; ++seed) {
        const world::Terrain terrain(seed);

        // When / Then
        for (int y = -world::SpawnClearRadius; y <= world::SpawnClearRadius; ++y) {
            for (int x = -world::SpawnClearRadius; x <= world::SpawnClearRadius; ++x) {
                ASSERT_FALSE(world::is_ore(terrain.tile_at({x, y})));
            }
        }
    }
}

TEST(Terrain, StartingNodesLieOnDryLandAwayFromTheWater) {
    // Given: many seeds, so some have lakes right next to the spawn
    for (std::uint64_t seed = 0; seed < 200; ++seed) {
        const world::Terrain terrain(seed);

        // When / Then: no starting ore tile touches water, so no ore is drawn in a lake
        for (int y = -45; y <= 45; ++y) {
            for (int x = -45; x <= 45; ++x) {
                if (!world::is_ore(terrain.tile_at({x, y}))) {
                    continue;
                }
                const world::TileCoord neighbours[] = {{x + 1, y}, {x - 1, y}, {x, y + 1}, {x, y - 1}};
                for (const world::TileCoord n : neighbours) {
                    ASSERT_NE(terrain.tile_at(n), world::TileType::Water)
                        << "seed " << seed << " tile " << x << "," << y;
                }
            }
        }
    }
}

TEST(Terrain, OreNodesGrowWithDistanceFromTheSpawn) {
    // Given / When
    const double near_average = average_node_size(70, -100, 200, 200, 12);
    const double far_average = average_node_size(500, -150, 400, 300, 12);

    // Then
    EXPECT_GT(near_average, 0.0);
    EXPECT_GT(far_average, near_average * 1.5);
}

TEST(Terrain, FarAwayWorldHasEveryOreType) {
    // Given
    for (std::uint64_t seed = 0; seed < 20; ++seed) {
        const world::Terrain terrain(seed);

        // When: looking outside the starting area
        std::array<bool, 4> found{};
        for (int y = 70; y < 500; ++y) {
            for (int x = 70; x < 500; ++x) {
                const world::TileType type = terrain.tile_at({x, y});
                if (world::is_ore(type)) {
                    found[ore_index(type)] = true;
                }
            }
        }

        // Then
        EXPECT_TRUE(found[0] && found[1] && found[2] && found[3]) << "seed " << seed;
    }
}

TEST(Terrain, NoScatteredNodesInsideTheStartingArea) {
    // Given
    for (std::uint64_t seed = 0; seed < 40; ++seed) {
        const world::Terrain terrain(seed);

        // When / Then: ore inside the starting area is only the three starting nodes, which
        // stay close to the spawn
        for (int y = -world::StartingAreaRadius; y <= world::StartingAreaRadius; ++y) {
            for (int x = -world::StartingAreaRadius; x <= world::StartingAreaRadius; ++x) {
                if (std::hypot(x, y) >= world::StartingAreaRadius || !world::is_ore(terrain.tile_at({x, y}))) {
                    continue;
                }
                ASSERT_LT(std::hypot(x, y), 45.0) << "seed " << seed << " tile " << x << "," << y;
            }
        }
    }
}
