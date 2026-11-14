#include <gtest/gtest.h>

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

TEST(Terrain, GrassAndWaterAreasAreLargeComparedToThePlayer) {
    // Given: the player is about one tile wide, so areas of a few tiles make it a giant
    constexpr int RowLength = 600;
    constexpr int Rows = 4;
    constexpr double MinAverageRun = 120.0;

    for (std::uint64_t seed = 0; seed < 10; ++seed) {
        const world::Terrain terrain(seed);

        // When: walking along rows away from the spawn clearing, counting runs of one type
        int runs = 0;
        for (int row = 40; row < 40 + Rows * 5; row += 5) {
            world::TileType previous = terrain.tile_at({-RowLength / 2, row});
            ++runs;
            for (int x = -RowLength / 2 + 1; x < RowLength / 2; ++x) {
                const world::TileType type = terrain.tile_at({x, row});
                runs += type != previous ? 1 : 0;
                previous = type;
            }
        }

        // Then
        const double average_run = static_cast<double>(Rows) * RowLength / runs;
        EXPECT_GE(average_run, MinAverageRun) << "seed " << seed;
    }
}
