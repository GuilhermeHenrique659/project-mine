#include <gtest/gtest.h>

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/systems/production_system.hpp"

namespace {

std::optional<world::TileCoord> find_tile(const world::Terrain& terrain, world::TileType type) {
    for (int x = -300; x < 300; ++x) {
        for (int y = -300; y < 300; ++y) {
            if (terrain.tile_at({x, y}) == type) {
                return world::TileCoord{x, y};
            }
        }
    }
    return std::nullopt;
}

struct Fixture {
    core::Registry registry;
    world::Terrain terrain{7};
    world::ProductionSystem system{registry, terrain};

    // A finished catalog mine on the first tile of `ore`.
    core::EntityId mine_on(world::TileType ore) {
        const auto tile = find_tile(terrain, ore);
        EXPECT_TRUE(tile);
        const core::EntityId id = registry.create("Mine");
        world::make_building(registry, id, *world::find_construction(world::BuildingType::Mine),
                             tile.value_or(world::TileCoord{}));
        registry.remove<world::ConstructionSite>(id);
        return id;
    }

    // A finished building with its own recipe and inventory on grass.
    core::EntityId producer(world::Recipe recipe, std::size_t slots) {
        const core::EntityId id = registry.create("Smelter");
        registry.add(id, world::Building{world::BuildingType::Mine, {1, 1}});
        registry.add(id, world::Inventory{slots});
        registry.add(id, world::Producer{recipe});
        return id;
    }

    int count(core::EntityId id, world::Item item) const {
        return registry.get<world::Inventory>(id)->count(item);
    }
};

} // namespace

TEST(ProductionSystem, TheMineRecipeNeedsNoInputAndYieldsTheGroundAtHalfAnItemPerSecond) {
    // Given / When
    const auto& recipe = world::find_construction(world::BuildingType::Mine)->recipe;

    // Then
    ASSERT_TRUE(recipe.has_value());
    EXPECT_FALSE(recipe->input.has_value());
    EXPECT_FALSE(recipe->output.has_value());
    EXPECT_FLOAT_EQ(recipe->rate, 0.5f);
}

TEST(ProductionSystem, AMineProducesTheOreUnderItAtItsRate) {
    // Given
    Fixture f;
    const core::EntityId iron = f.mine_on(world::TileType::IronOre);
    const core::EntityId coal = f.mine_on(world::TileType::Coal);

    // When
    f.system.update(4.0f);

    // Then
    EXPECT_EQ(f.count(iron, world::items::IronOre), 2);
    EXPECT_EQ(f.count(coal, world::items::Coal), 2);
}

TEST(ProductionSystem, PartialProgressCarriesOverToTheNextFrame) {
    // Given
    Fixture f;
    const core::EntityId mine = f.mine_on(world::TileType::CopperOre);

    // When
    f.system.update(1.5f);
    const int after_first = f.count(mine, world::items::CopperOre);
    f.system.update(0.5f);

    // Then
    EXPECT_EQ(after_first, 0);
    EXPECT_EQ(f.count(mine, world::items::CopperOre), 1);
}

TEST(ProductionSystem, AConstructionSiteProducesNothing) {
    // Given
    Fixture f;
    const core::EntityId mine = f.mine_on(world::TileType::IronOre);
    f.registry.add(mine, world::ConstructionSite{0.0f, 300.0f});

    // When
    f.system.update(10.0f);

    // Then
    EXPECT_EQ(f.count(mine, world::items::IronOre), 0);
}

TEST(ProductionSystem, AFullInventoryWaitsWithAtMostOneItemOfProgress) {
    // Given: one slot, already full of iron ore
    Fixture f;
    const core::EntityId id = f.producer({std::nullopt, world::items::IronOre, 1.0f}, 1);
    f.registry.get<world::Inventory>(id)->add(world::items::IronOre, 100);

    // When
    f.system.update(5.0f);
    f.registry.get<world::Inventory>(id)->remove(world::items::IronOre, 10);
    f.system.update(0.0f);

    // Then: the waiting item comes out as soon as there is room, the rest was not banked
    EXPECT_EQ(f.count(id, world::items::IronOre), 91);
}

TEST(ProductionSystem, AnInputIsConsumedForEachOutputAndItsLackStopsProduction) {
    // Given: 2 iron ore -> 1 coal, with iron for one output only
    Fixture f;
    const core::EntityId id =
        f.producer({world::ItemStack{world::items::IronOre, 2}, world::items::Coal, 1.0f}, 4);
    f.registry.get<world::Inventory>(id)->add(world::items::IronOre, 3);

    // When
    f.system.update(3.0f);

    // Then
    EXPECT_EQ(f.count(id, world::items::Coal), 1);
    EXPECT_EQ(f.count(id, world::items::IronOre), 1);
}
