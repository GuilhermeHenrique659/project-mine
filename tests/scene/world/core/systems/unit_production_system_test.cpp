#include <gtest/gtest.h>

#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/systems/unit_production_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

// A finished, fully staffed Tank Factory on (1, 1) holding the parts of `tanks` tanks, run
// by a production system that can spawn entities.
struct TankFactoryFixture {
    core::Registry registry;
    world::Terrain terrain{7};
    world::EntityFactory factory = world::make_entity_factory();
    world::UnitProductionSystem spawning{registry, terrain, factory};
    core::EntityId id = 0;

    explicit TankFactoryFixture(int tanks, bool walled_in = false) {
        world::track_buildings(registry);
        if (walled_in) {
            // A wall over every tile the factory could spawn on; the factory sits inside it.
            const core::EntityId wall = registry.create("Wall");
            registry.add(wall, world::Building{world::BuildingType::Storage, {-9, -9}, {21, 21}});
        }
        const world::Construction& entry = *world::find_construction(world::BuildingType::TankFactory);
        id = registry.create("Tank Factory");
        world::make_building(registry, id, entry, {1, 1});
        registry.remove<world::ConstructionSite>(id);
        staff(id, entry.blueprint.find<world::Staffing>()->needed);
        auto& inventory = *registry.get<world::SolidInventory>(id);
        inventory.add(world::items::Engine, 12 * tanks);
        inventory.add(world::items::Weapon, 5 * tanks);
        inventory.add(world::items::Steel, 200 * tanks);
        inventory.add(world::items::Electronics, 200 * tanks);
        inventory.add(world::items::Tool, 100 * tanks);
    }

    // `count` workers assigned to `building`, standing on it.
    void staff(core::EntityId building, int count) {
        const world::WorldPoint at = registry.get<world::Position>(building)->point;
        for (int i = 0; i < count; ++i) {
            const core::EntityId worker = registry.create("Worker");
            registry.add(worker, world::Worker{world::WorkerState::Working, building});
            registry.add(worker, world::Position{at});
        }
    }

    int count(core::EntityId building, world::Item item) const {
        return registry.get<world::SolidInventory>(building)->count(item);
    }

    std::vector<core::EntityId> tanks() const {
        std::vector<core::EntityId> found;
        for (const core::Entity& entity : registry.entities()) {
            if (registry.has<world::BattleTank>(entity.id)) {
                found.push_back(entity.id);
            }
        }
        return found;
    }

    void run(int seconds) {
        for (int second = 0; second < seconds; ++second) {
            spawning.update(1.0f);
        }
    }
};

} // namespace

TEST(UnitProductionSystem, AStaffedTankFactoryRollsOutABattleTankBesideItAfterTwoMinutes) {
    // Given
    TankFactoryFixture f(1);

    // When: 119 s is not quite a tank, 120 s is
    f.run(119);
    const std::size_t before = f.tanks().size();
    f.run(1);

    // Then: on a free tile just outside the 3x3 footprint (1..3), every part used up
    EXPECT_EQ(before, 0u);
    ASSERT_EQ(f.tanks().size(), 1u);
    const world::TileCoord tile =
        world::tile_containing(f.registry.get<world::Position>(f.tanks()[0])->point);
    EXPECT_EQ(world::building_at(f.registry, tile), 0u);
    EXPECT_GE(tile.x, 0);
    EXPECT_LE(tile.x, 4);
    EXPECT_GE(tile.y, 0);
    EXPECT_LE(tile.y, 4);
    EXPECT_EQ(f.count(f.id, world::items::Engine), 0);
    EXPECT_EQ(f.count(f.id, world::items::Weapon), 0);
    EXPECT_EQ(f.count(f.id, world::items::Steel), 0);
    EXPECT_EQ(f.count(f.id, world::items::Electronics), 0);
    EXPECT_EQ(f.count(f.id, world::items::Tool), 0);
}

TEST(UnitProductionSystem, ATankFactoryWithNoFreeTileAroundItStopsAndKeepsItsParts) {
    // Given
    TankFactoryFixture f(1, true);

    // When
    f.run(240);

    // Then
    EXPECT_TRUE(f.tanks().empty());
    EXPECT_EQ(f.count(f.id, world::items::Engine), 12);
    EXPECT_EQ(f.registry.get<world::BuildingStatus>(f.id)->reason, world::IdleReason::OutputFull);
}

TEST(UnitProductionSystem, ATankFactoryMissingAPartMakesNoTank) {
    // Given
    TankFactoryFixture f(1);
    f.registry.get<world::SolidInventory>(f.id)->remove(world::items::Weapon, 1);

    // When
    f.run(240);

    // Then
    EXPECT_TRUE(f.tanks().empty());
    EXPECT_EQ(f.registry.get<world::BuildingStatus>(f.id)->reason, world::IdleReason::MissingInput);
}
