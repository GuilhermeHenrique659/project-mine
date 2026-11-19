#include <gtest/gtest.h>

#include "scene/world/core/terrain/terrain.hpp"

#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/save/world_serializer.hpp"
#include "scene/world/core/systems/worker_system.hpp"

namespace {

constexpr world::TileCoord HqTile{0, 0};
constexpr world::TileCoord MineTile{4, 0};
constexpr world::TileCoord SiteTile{0, 6};

// A world saved and loaded: the second registry is filled from the first one's snapshot.
struct Fixture {
    world::EntityFactory factory = world::make_entity_factory();
    world::WorldSerializer serializer = world::make_world_serializer(factory);
    core::Registry saved;
    core::Registry loaded;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::WorkerSystem system{loaded, bus, factory, terrain};

    core::EntityId build(world::BuildingType type, world::TileCoord tile, bool finished) {
        const core::EntityId id = *factory.spawn(saved, world::BuildingArchetype, "Building");
        world::make_building(saved, id, *world::find_construction(type), tile);
        if (finished) {
            saved.remove<world::ConstructionSite>(id);
        }
        return id;
    }

    core::EntityId worker_at(world::WorldPoint at) {
        const core::EntityId id = *factory.spawn(saved, world::WorkerArchetype, "Worker");
        saved.get<world::Position>(id)->point = at;
        return id;
    }

    // Saves, loads and lets the worker system run once on the loaded world.
    void save_and_load() {
        serializer.restore(loaded, serializer.snapshot(saved));
        system.update(0.0f);
    }

    core::EntityId loaded_building(world::TileCoord tile) const {
        for (const core::Entity& entity : loaded.entities()) {
            const auto* building = loaded.get<world::Building>(entity.id);
            if (building != nullptr && building->tile == tile) return entity.id;
        }
        return 0;
    }

    std::vector<core::EntityId> loaded_workers() {
        std::vector<core::EntityId> ids;
        loaded.each<world::Worker>([&](core::EntityId id, world::Worker&) { ids.push_back(id); });
        return ids;
    }
};

} // namespace

TEST(WorkerSave, AWorkerAtItsMineLoadsWorkingThereAtTheSamePosition) {
    // Given
    Fixture f;
    const auto hq = f.build(world::BuildingType::Headquarters, HqTile, true);
    const auto mine = f.build(world::BuildingType::Mine, MineTile, true);
    const auto worker = f.worker_at({4.25f, 0.75f});
    *f.saved.get<world::Worker>(worker) = {world::WorkerState::Working, mine};
    f.saved.add(worker, world::RouteFollower{{hq, mine}});

    // When
    f.save_and_load();

    // Then
    const auto workers = f.loaded_workers();
    ASSERT_EQ(workers.size(), 1u);
    const auto* loaded = f.loaded.get<world::Worker>(workers[0]);
    EXPECT_EQ(loaded->state, world::WorkerState::Working);
    EXPECT_EQ(loaded->workplace, f.loaded_building(MineTile));
    const auto& at = f.loaded.get<world::Position>(workers[0])->point;
    EXPECT_FLOAT_EQ(at.x, 4.25f);
    EXPECT_FLOAT_EQ(at.y, 0.75f);
    const auto* follower = f.loaded.get<world::RouteFollower>(workers[0]);
    ASSERT_NE(follower, nullptr);
    EXPECT_EQ(follower->route,
              (world::RouteKey{f.loaded_building(HqTile), f.loaded_building(MineTile)}));
    EXPECT_TRUE(follower->forward);
}

TEST(WorkerSave, AWorkerOnTheWayToASiteLoadsStillGoingToBuildIt) {
    // Given: the site is saved after the worker, so it is loaded after it too; another site
    // comes first, the one a freshly idle worker would take
    Fixture f;
    const auto hq = f.build(world::BuildingType::Headquarters, HqTile, true);
    f.build(world::BuildingType::Mine, {0, -6}, false);
    const auto worker = f.worker_at({0.5f, 3.0f});
    const auto site = f.build(world::BuildingType::Mine, SiteTile, false);
    f.saved.get<world::Worker>(worker)->state = world::WorkerState::Working;
    f.saved.get<world::Builder>(worker)->site = site;
    f.saved.add(worker, world::RouteFollower{{hq, site}});

    // When
    f.save_and_load();

    // Then
    const auto workers = f.loaded_workers();
    ASSERT_EQ(workers.size(), 1u);
    EXPECT_EQ(f.loaded.get<world::Worker>(workers[0])->state, world::WorkerState::Working);
    EXPECT_EQ(f.loaded.get<world::Builder>(workers[0])->site, f.loaded_building(SiteTile));
    const auto* follower = f.loaded.get<world::RouteFollower>(workers[0]);
    ASSERT_NE(follower, nullptr);
    EXPECT_EQ(follower->route.to, f.loaded_building(SiteTile));
}

TEST(WorkerSave, AnIdleWorkerWalkingBackKeepsWalkingBackAfterLoading) {
    // Given
    Fixture f;
    const auto hq = f.build(world::BuildingType::Headquarters, HqTile, true);
    const auto mine = f.build(world::BuildingType::Mine, MineTile, true);
    const auto worker = f.worker_at({2.5f, 0.5f});
    f.saved.add(worker, world::RouteFollower{{hq, mine}, world::RouteEnd::Stop, 2, false});

    // When
    f.save_and_load();

    // Then
    const auto workers = f.loaded_workers();
    ASSERT_EQ(workers.size(), 1u);
    EXPECT_EQ(f.loaded.get<world::Worker>(workers[0])->state, world::WorkerState::Idle);
    const auto* follower = f.loaded.get<world::RouteFollower>(workers[0]);
    ASSERT_NE(follower, nullptr);
    EXPECT_EQ(follower->route.to, f.loaded_building(MineTile));
    EXPECT_FALSE(follower->forward);
}

TEST(WorkerSave, AWorkerWhoseSavedJobIsNoLongerThereLoadsIdle) {
    // Given: a record pointing at a workplace on an empty tile
    Fixture f;
    f.build(world::BuildingType::Headquarters, HqTile, true);
    std::vector<game::EntityRecord> records = f.serializer.snapshot(f.saved);
    records.push_back({world::WorkerArchetype, "Worker",
                       {{"position.x", "1"}, {"position.y", "1"}, {"worker.state", "Working"},
                        {"worker.workplace.x", "9"}, {"worker.workplace.y", "9"}}});

    // When
    f.serializer.restore(f.loaded, records);
    f.system.update(0.0f);

    // Then
    const auto workers = f.loaded_workers();
    ASSERT_EQ(workers.size(), 1u);
    EXPECT_EQ(f.loaded.get<world::Worker>(workers[0])->state, world::WorkerState::Idle);
    EXPECT_EQ(f.loaded.get<world::Worker>(workers[0])->workplace, 0u);
}

TEST(SoldierSave, ASoldierKeepsItsTwoSlotInventoryThroughASaveAndLoad) {
    // Given
    Fixture f;
    const core::EntityId soldier = *f.factory.spawn(f.saved, world::SoldierArchetype, "Soldier");
    f.saved.get<world::Inventory>(soldier)->add(world::items::Weapon, 1);
    f.saved.get<world::Inventory>(soldier)->add(world::items::Ammo, 50);

    // When
    f.save_and_load();

    // Then
    int soldiers = 0;
    f.loaded.each<world::Soldier, world::Inventory>(
        [&](core::EntityId, world::Soldier&, world::Inventory& inventory) {
            ++soldiers;
            EXPECT_EQ(inventory.capacity(), 2u);
            EXPECT_EQ(inventory.count(world::items::Weapon), 1);
            EXPECT_EQ(inventory.count(world::items::Ammo), 50);
        });
    EXPECT_EQ(soldiers, 1);
}
