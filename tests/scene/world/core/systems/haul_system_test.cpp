#include <gtest/gtest.h>

#include "scene/world/core/occupancy.hpp"

#include <memory>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/events/worker_events.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/systems/haul_system.hpp"
#include "scene/world/core/systems/route_follow_system.hpp"
#include "scene/world/core/systems/unit_traffic_system.hpp"
#include "scene/world/core/systems/worker_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

constexpr float Frame = 0.05f;

// A headquarters at (0, 0), a storage at (3, 0) and a mine at (6, 0), each with a
// 10-slot inventory, on grass, with every route computed.
struct Fixture {
    Fixture() {
        hq = add(world::BuildingType::Headquarters, {0, 0});
        storage = add(world::BuildingType::Storage, {3, 0});
        mine = add(world::BuildingType::Mine, {6, 0});
        registry.add(mine, world::Producer{});
        network.rebuild(registry, terrain);
        route = {storage, mine};
    }

    core::EntityId add(world::BuildingType type, world::TileCoord tile) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, tile});
        registry.add(id, world::Position{{tile.x + 0.5f, tile.y + 0.5f}});
        registry.add(id, world::SolidInventory{10});
        return id;
    }

    core::EntityId add_worker() { return world::spawn_worker(registry, "Worker", {0.5f, 0.5f}); }

    world::SolidInventory& inventory(core::EntityId id) { return *registry.get<world::SolidInventory>(id); }

    void run(int frames) {
        for (int i = 0; i < frames; ++i) {
            workers.update(Frame);
            follow.update(Frame);
            move.update(Frame);
            haul.update(Frame);
        }
    }

    int haulers() {
        int count = 0;
        registry.each<world::Hauler>([&](core::EntityId, world::Hauler&) { ++count; });
        return count;
    }

    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::EntityFactory factory = world::make_entity_factory();
    world::RouteNetwork network{world::default_route_planner()};
    world::WorkerSystem workers{registry, bus, factory, terrain};
    world::RouteFollowSystem follow{registry, network};
    world::CharacterMoveSystem move{registry, bus};
    world::HaulSystem haul{registry, bus, network};
    core::EntityId hq = 0;
    core::EntityId storage = 0;
    core::EntityId mine = 0;
    world::RouteKey route;
};

} // namespace

TEST(HaulSystem, RequestingHaulersMakesIdleWorkersHaulOnTheRoute) {
    // Given
    Fixture f;
    const auto a = f.add_worker();
    const auto b = f.add_worker();
    const auto c = f.add_worker();

    // When
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Put, 2});

    // Then
    EXPECT_EQ(f.haulers(), 2);
    for (const auto id : {a, b}) {
        EXPECT_EQ(f.registry.get<world::Worker>(id)->state, world::WorkerState::Working);
        ASSERT_NE(f.registry.get<world::Hauler>(id), nullptr);
        EXPECT_EQ(f.registry.get<world::Hauler>(id)->route, f.route);
        EXPECT_EQ(f.registry.get<world::Hauler>(id)->mode, world::HaulMode::Put);
    }
    EXPECT_EQ(f.registry.get<world::Worker>(c)->state, world::WorkerState::Idle);
}

TEST(HaulSystem, ARequestNeverTakesMoreThanTheIdleWorkers) {
    // Given
    Fixture f;
    f.add_worker();

    // When
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 5});

    // Then
    EXPECT_EQ(f.haulers(), 1);
}

TEST(HaulSystem, ARequestForARouteThatIsNotLogisticIsIgnored) {
    // Given
    Fixture f;
    f.add_worker();

    // When
    f.bus.publish(world::HaulersRequested{{f.hq, f.mine}, world::HaulMode::Take, 1});

    // Then
    EXPECT_EQ(f.haulers(), 0);
}

TEST(HaulSystem, TakeModeCarriesTheProducersItemsToTheStorageTripAfterTrip) {
    // Given: 250 coal in the mine, three trips of one stack
    Fixture f;
    f.add_worker();
    f.inventory(f.mine).add(world::items::Coal, 250);
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 1});

    // When
    f.run(600);

    // Then
    EXPECT_EQ(f.inventory(f.mine).count(world::items::Coal), 0);
    EXPECT_EQ(f.inventory(f.storage).count(world::items::Coal), 250);
}

TEST(HaulSystem, PutModeCarriesTheStoragesItemsToTheProducer) {
    // Given
    Fixture f;
    f.add_worker();
    f.inventory(f.storage).add(world::items::IronOre, 150);
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Put, 1});

    // When
    f.run(600);

    // Then
    EXPECT_EQ(f.inventory(f.storage).count(world::items::IronOre), 0);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::IronOre), 150);
}

TEST(HaulSystem, AHaulerCarriesWhatFitsInItsOneSlotPerTrip) {
    // Given: 150 coal, more than one stack (100)
    Fixture f;
    const auto worker = f.add_worker();
    f.inventory(f.mine).add(world::items::Coal, 150);
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 1});

    // When: walk until it has loaded once
    int carried = 0;
    for (int i = 0; i < 600 && carried == 0; ++i) {
        f.run(1);
        carried = f.inventory(worker).count(world::items::Coal);
    }

    // Then
    EXPECT_EQ(carried, world::items::Coal.max_stack);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::Coal), 50);
}

TEST(Worker, HasAnInventoryOfOneSlot) {
    // Given
    core::Registry registry;

    // When
    const auto worker = world::spawn_worker(registry, "Worker", {0.5f, 0.5f});

    // Then
    ASSERT_NE(registry.get<world::SolidInventory>(worker), nullptr);
    EXPECT_EQ(registry.get<world::SolidInventory>(worker)->capacity(), world::WorkerInventorySlots);
    EXPECT_EQ(world::WorkerInventorySlots, 1u);
}

TEST(HaulSystem, AHaulerKeepsLoopingOnItsRoute) {
    // Given: nothing to carry
    Fixture f;
    const auto worker = f.add_worker();
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 1});

    // When
    f.run(600);

    // Then
    const auto* follower = f.registry.get<world::RouteFollower>(worker);
    ASSERT_NE(follower, nullptr);
    EXPECT_EQ(follower->route, f.route);
    EXPECT_EQ(follower->end, world::RouteEnd::Loop);
    EXPECT_EQ(f.registry.get<world::Worker>(worker)->state, world::WorkerState::Working);
}

TEST(HaulSystem, ReleasingHaulersSendsThemBackIdle) {
    // Given
    Fixture f;
    f.add_worker();
    f.add_worker();
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 2});

    // When
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, -1});

    // Then
    EXPECT_EQ(f.haulers(), 1);
    int idle = 0;
    f.registry.each<world::Worker>([&](core::EntityId id, world::Worker& worker) {
        if (worker.state != world::WorkerState::Idle) return;
        ++idle;
        const auto* follower = f.registry.get<world::RouteFollower>(id);
        ASSERT_NE(follower, nullptr);
        EXPECT_EQ(follower->route.from, f.hq);
        EXPECT_FALSE(follower->forward);
    });
    EXPECT_EQ(idle, 1);
}

TEST(HaulSystem, ChangingTheModeSwitchesEveryHaulerOfTheRoute) {
    // Given
    Fixture f;
    f.add_worker();
    f.add_worker();
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 2});

    // When
    f.bus.publish(world::HaulModeRequested{f.route, world::HaulMode::Put});

    // Then
    f.registry.each<world::Hauler>([&](core::EntityId, world::Hauler& hauler) {
        EXPECT_EQ(hauler.mode, world::HaulMode::Put);
    });
}

TEST(HaulSystem, AHaulerWhoseRouteIsGoneTurnsIdle) {
    // Given
    Fixture f;
    const auto worker = f.add_worker();
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 1});
    f.run(10);

    // When
    f.registry.destroy(f.mine);
    f.network.rebuild(f.registry, f.terrain);
    f.run(1);

    // Then
    EXPECT_FALSE(f.registry.has<world::Hauler>(worker));
    EXPECT_EQ(f.registry.get<world::Worker>(worker)->state, world::WorkerState::Idle);
}

TEST(HaulSystem, HaulersStillLoadAndUnloadWhenFramesAreLong) {
    // Given: frames four times longer than usual, so a hauler turning at an end can be
    // out of the end tile again within one frame
    Fixture f;
    f.add_worker();
    f.inventory(f.mine).add(world::items::Coal, 250);
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 1});

    // When
    for (int i = 0; i < 300; ++i) {
        f.workers.update(0.2f);
        f.follow.update(0.2f);
        f.move.update(0.2f);
        f.haul.update(0.2f);
    }

    // Then
    EXPECT_EQ(f.inventory(f.storage).count(world::items::Coal), 250);
}

TEST(HaulSystem, SeveralHaulersOnOneRouteWithCollisionAndTrafficStillDeliverEverything) {
    // Given: three haulers on the same corridor, walking both ways, with building and unit
    // collision as in the game
    Fixture f;
    for (int i = 0; i < 3; ++i) f.add_worker();
    f.inventory(f.mine).add(world::items::Coal, 250);
    f.inventory(f.storage) = world::SolidInventory{10};
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 3});
    world::UnitTrafficSystem traffic{f.registry};
    world::CharacterMoveSystem colliding{f.registry, f.bus, &f.terrain};

    // When
    for (int i = 0; i < 1200; ++i) {
        f.workers.update(Frame);
        f.follow.update(Frame);
        traffic.update(Frame);
        colliding.update(Frame);
        f.haul.update(Frame);
    }

    // Then: the coal ends up in the storage, nothing is stuck in a hauler
    int carried = 0;
    f.registry.each<world::Hauler>([&](core::EntityId id, world::Hauler&) {
        carried += f.inventory(id).count(world::items::Coal);
    });
    EXPECT_EQ(f.inventory(f.storage).count(world::items::Coal) + carried, 250);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::Coal), 0);
}

TEST(HaulSystem, WorkersWaitingAtTheHeadquartersSpreadOverItWithoutLeaving) {
    // Given: a crowd of idle workers on one spot, with traffic and collision running
    Fixture f;
    for (int i = 0; i < 100; ++i) f.add_worker();
    world::UnitTrafficSystem traffic{f.registry};
    world::CharacterMoveSystem colliding{f.registry, f.bus, &f.terrain};

    // When
    for (int i = 0; i < 100; ++i) {
        f.workers.update(Frame);
        f.follow.update(Frame);
        traffic.update(Frame);
        colliding.update(Frame);
    }

    // Then: none left the headquarters, and they no longer share one spot
    int on_the_spot = 0;
    f.registry.each<world::Worker>([&](core::EntityId id, world::Worker&) {
        const auto& at = f.registry.get<world::Position>(id)->point;
        EXPECT_EQ(world::building_at(f.registry, world::tile_containing(at)), f.hq);
        if (at.x == 0.5f && at.y == 0.5f) ++on_the_spot;
    });
    EXPECT_LT(on_the_spot, 100);
}

TEST(HaulSystem, TakeModeLeavesTheProducersFilteredInputSlotsAlone) {
    // Given: a producer with iron in an input slot and steel in its general slots
    Fixture f;
    f.add_worker();
    world::SolidInventory producer{2};
    producer.append(1, world::ItemType::IronOre);
    producer.add(world::items::IronOre, 40);
    producer.add(world::items::Steel, 30);
    f.inventory(f.mine) = producer;
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 1});

    // When
    f.run(600);

    // Then
    EXPECT_EQ(f.inventory(f.storage).count(world::items::Steel), 30);
    EXPECT_EQ(f.inventory(f.storage).count(world::items::IronOre), 0);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::IronOre), 40);
}

TEST(HaulSystem, PutModeCarriesOnlyTheItemsTheProducerTakesAsInputs) {
    // Given: coal stored before the copper, a producer whose only input is copper ore
    Fixture f;
    f.add_worker();
    f.inventory(f.storage).add(world::items::Coal, 100);
    f.inventory(f.storage).add(world::items::CopperOre, 150);
    world::SolidInventory producer{2};
    producer.append(2, world::ItemType::CopperOre);
    f.inventory(f.mine) = producer;
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Put, 1});

    // When
    f.run(600);

    // Then
    EXPECT_EQ(f.inventory(f.mine).count(world::items::CopperOre), 150);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::Coal), 0);
    EXPECT_EQ(f.inventory(f.storage).count(world::items::Coal), 100);
}

TEST(HaulSystem, PutModeLeavesAnInputInTheStorageWhileTheProducersSectionIsFull) {
    // Given: the producer's only copper slot is full
    Fixture f;
    const auto worker = f.add_worker();
    f.inventory(f.storage).add(world::items::CopperOre, 80);
    world::SolidInventory producer{2};
    producer.append(1, world::ItemType::CopperOre);
    producer.add(world::items::CopperOre, 100);
    f.inventory(f.mine) = producer;
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Put, 1});

    // When
    f.run(600);

    // Then
    EXPECT_EQ(f.inventory(f.storage).count(world::items::CopperOre), 80);
    EXPECT_TRUE(f.inventory(worker).is_empty());
}

TEST(HaulSystem, TakeAndPutBringsTheInputsAndCarriesTheOutputBack) {
    // Given: iron and coal in the storage, steel in a steelworks-like producer
    Fixture f;
    f.add_worker();
    f.inventory(f.storage).add(world::items::IronOre, 50);
    f.inventory(f.storage).add(world::items::Coal, 50);
    world::SolidInventory producer{2};
    producer.append(1, world::ItemType::IronOre).append(1, world::ItemType::Coal);
    producer.add(world::items::Steel, 150);
    f.inventory(f.mine) = producer;
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Both, 1});

    // When
    f.run(1200);

    // Then
    EXPECT_EQ(f.inventory(f.mine).count(world::items::IronOre), 50);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::Coal), 50);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::Steel), 0);
    EXPECT_EQ(f.inventory(f.storage).count(world::items::Steel), 150);
}

TEST(HaulSystem, TakeModeNeverUnloadsIntoTheProducer) {
    // Given: a Take hauler still carrying steel (the storage was full)
    Fixture f;
    const auto worker = f.add_worker();
    f.inventory(f.storage) = world::SolidInventory{0};
    f.inventory(f.mine).add(world::items::Steel, 40);
    f.bus.publish(world::HaulersRequested{f.route, world::HaulMode::Take, 1});

    // When
    f.run(600);

    // Then: it keeps the steel instead of dropping it back
    EXPECT_EQ(f.inventory(worker).count(world::items::Steel), 40);
    EXPECT_EQ(f.inventory(f.mine).count(world::items::Steel), 0);
}
