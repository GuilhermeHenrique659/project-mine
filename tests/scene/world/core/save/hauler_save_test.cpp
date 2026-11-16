#include <gtest/gtest.h>

#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/save/world_serializer.hpp"
#include "scene/world/core/systems/haul_system.hpp"
#include "scene/world/core/systems/worker_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

constexpr world::TileCoord HqTile{-6, 0};
constexpr world::TileCoord StorageTile{0, 0};
constexpr world::TileCoord MineTile{4, 0};

// A headquarters, a storage and a mine saved with one hauler, then loaded into a second
// registry where the routes are rebuilt and the systems run once (as in the game).
struct Fixture {
    Fixture() {
        build(world::BuildingType::Headquarters, HqTile);
        storage = build(world::BuildingType::Storage, StorageTile);
        mine = build(world::BuildingType::Mine, MineTile);
        worker = *factory.spawn(saved, world::WorkerArchetype, "Worker");
        saved.get<world::Position>(worker)->point = {1.5f, 0.5f};
    }

    core::EntityId build(world::BuildingType type, world::TileCoord tile) {
        const core::EntityId id = *factory.spawn(saved, world::BuildingArchetype, "Building");
        world::make_building(saved, id, *world::find_construction(type), tile);
        saved.remove<world::ConstructionSite>(id);
        return id;
    }

    void make_hauler(world::HaulMode mode, bool joined) {
        saved.get<world::Worker>(worker)->state = world::WorkerState::Working;
        saved.add(worker, world::Hauler{{storage, mine}, mode, joined});
        world::RouteFollower follower{{storage, mine}, world::RouteEnd::Loop};
        follower.forward = false;
        saved.add(worker, follower);
    }

    void save() { serializer.restore(loaded, serializer.snapshot(saved)); }

    void load_systems() {
        network.rebuild(loaded, terrain);
        workers.update(0.0f);
        haul.update(0.0f);
    }

    core::EntityId loaded_building(world::TileCoord tile) const {
        for (const core::Entity& entity : loaded.entities()) {
            const auto* building = loaded.get<world::Building>(entity.id);
            if (building != nullptr && building->tile == tile) return entity.id;
        }
        return 0;
    }

    core::EntityId loaded_worker() {
        core::EntityId found = 0;
        loaded.each<world::Worker>([&](core::EntityId id, world::Worker&) { found = id; });
        return found;
    }

    world::RouteKey loaded_route() const {
        return {loaded_building(StorageTile), loaded_building(MineTile)};
    }

    world::EntityFactory factory = world::make_entity_factory();
    world::WorldSerializer serializer = world::make_world_serializer(factory);
    world::Terrain terrain{7};
    core::Registry saved;
    core::Registry loaded;
    event::EventBus bus;
    world::RouteNetwork network{world::default_route_planner()};
    world::WorkerSystem workers{loaded, bus, factory};
    world::HaulSystem haul{loaded, bus, network};
    core::EntityId storage = 0;
    core::EntityId mine = 0;
    core::EntityId worker = 0;
};

} // namespace

TEST(HaulerSave, AHaulerOnItsRouteLoadsLoopingOnItInItsMode) {
    // Given
    Fixture f;
    f.make_hauler(world::HaulMode::Put, true);

    // When
    f.save();
    f.load_systems();

    // Then
    const auto id = f.loaded_worker();
    EXPECT_EQ(f.loaded.get<world::Worker>(id)->state, world::WorkerState::Working);
    const auto* hauler = f.loaded.get<world::Hauler>(id);
    ASSERT_NE(hauler, nullptr);
    EXPECT_EQ(hauler->route, f.loaded_route());
    EXPECT_EQ(hauler->mode, world::HaulMode::Put);
    EXPECT_TRUE(hauler->joined);
    const auto* follower = f.loaded.get<world::RouteFollower>(id);
    ASSERT_NE(follower, nullptr);
    EXPECT_EQ(follower->route, f.loaded_route());
    EXPECT_EQ(follower->end, world::RouteEnd::Loop);
    EXPECT_FALSE(follower->forward);
}

TEST(HaulerSave, AHaulerStillWalkingToTheStorageLoadsGoingThere) {
    // Given
    Fixture f;
    f.make_hauler(world::HaulMode::Take, false);

    // When
    f.save();
    f.load_systems();

    // Then
    const auto id = f.loaded_worker();
    const auto* hauler = f.loaded.get<world::Hauler>(id);
    ASSERT_NE(hauler, nullptr);
    EXPECT_FALSE(hauler->joined);
    EXPECT_EQ(hauler->mode, world::HaulMode::Take);
    const auto* follower = f.loaded.get<world::RouteFollower>(id);
    ASSERT_NE(follower, nullptr);
    EXPECT_EQ(follower->route,
              (world::RouteKey{f.loaded_building(HqTile), f.loaded_building(StorageTile)}));
    EXPECT_TRUE(follower->forward);
}

TEST(HaulerSave, AHaulerKeepsWhatItCarries) {
    // Given
    Fixture f;
    f.make_hauler(world::HaulMode::Take, true);
    f.saved.get<world::Inventory>(f.worker)->add(world::items::Coal, 60);

    // When
    f.save();
    f.load_systems();

    // Then
    const auto* carried = f.loaded.get<world::Inventory>(f.loaded_worker());
    ASSERT_NE(carried, nullptr);
    EXPECT_EQ(carried->count(world::items::Coal), 60);
}

TEST(HaulerSave, AHaulerWhoseBuildingIsGoneLoadsIdle) {
    // Given
    Fixture f;
    f.make_hauler(world::HaulMode::Take, true);
    f.save();
    f.loaded.destroy(f.loaded_building(MineTile));

    // When
    f.load_systems();

    // Then
    const auto id = f.loaded_worker();
    EXPECT_FALSE(f.loaded.has<world::Hauler>(id));
    EXPECT_EQ(f.loaded.get<world::Worker>(id)->state, world::WorkerState::Idle);
}
