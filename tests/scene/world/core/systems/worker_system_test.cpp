#include <gtest/gtest.h>

#include <memory>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/persistent.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/core/events/worker_events.hpp"
#include "scene/world/core/worker_report.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/systems/build_system.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/systems/route_follow_system.hpp"
#include "scene/world/core/systems/worker_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

constexpr float Frame = 0.05f;
constexpr world::TileCoord HqTile{0, 0};
constexpr world::TileCoord SiteTile{4, 0};

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::EntityFactory factory = world::make_entity_factory();
    world::WorkerSystem system{registry, bus, factory};

    core::EntityId add_building(world::BuildingType type, world::TileCoord tile) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, tile});
        registry.add(id, world::Position{{tile.x + 0.5f, tile.y + 0.5f}});
        return id;
    }

    core::EntityId add_site(world::TileCoord tile, float effort) {
        const core::EntityId id = add_building(world::BuildingType::Mine, tile);
        registry.add(id, world::ConstructionSite{0.0f, effort});
        return id;
    }

    // A finished building that needs `needed` workers to produce.
    core::EntityId add_workplace(world::TileCoord tile, int needed) {
        const core::EntityId id = add_building(world::BuildingType::Mine, tile);
        registry.add(id, world::Staffing{needed});
        return id;
    }

    core::EntityId add_worker(world::WorldPoint at) {
        return world::spawn_worker(registry, "Worker", at);
    }

    int workers_in(world::WorkerState state) {
        int count = 0;
        registry.each<world::Worker>([&](core::EntityId, world::Worker& worker) {
            count += worker.state == state ? 1 : 0;
        });
        return count;
    }
};

} // namespace

TEST(WorkerSystem, ASiteTakesOneWorkerPerTenEffort) {
    // Given
    const world::ConstructionSite mine{0.0f, 300.0f};
    const world::ConstructionSite tiny{0.0f, 5.0f};

    // When
    const int mine_workers = world::max_workers(mine);
    const int tiny_workers = world::max_workers(tiny);

    // Then
    EXPECT_EQ(mine_workers, 30);
    EXPECT_EQ(tiny_workers, 0);
}

TEST(WorkerSystem, PlacingTheHeadquartersSpawnsAHundredIdleWorkersOnIt) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);

    // When
    f.bus.publish(world::BuildingPlaced{HqTile});

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Idle), world::HeadquartersWorkers);
    f.registry.each<world::Worker, world::Position, world::Persistent>(
        [](core::EntityId, world::Worker&, world::Position& at, world::Persistent& persistent) {
            EXPECT_FLOAT_EQ(at.point.x, 0.5f);
            EXPECT_FLOAT_EQ(at.point.y, 0.5f);
            EXPECT_EQ(persistent.archetype, world::WorkerArchetype);
        });
}

TEST(WorkerSystem, PlacingAnotherBuildingSpawnsNoWorkers) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    f.add_site(SiteTile, 300.0f);

    // When
    f.bus.publish(world::BuildingPlaced{SiteTile});

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Idle), 0);
}

TEST(WorkerSystem, AnIdleWorkerTakesASiteAndFollowsTheRouteFromTheHeadquarters) {
    // Given
    Fixture f;
    const auto hq = f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto site = f.add_site(SiteTile, 300.0f);
    const auto worker = f.add_worker({0.5f, 0.5f});

    // When
    f.system.update(Frame);

    // Then
    EXPECT_EQ(f.registry.get<world::Worker>(worker)->state, world::WorkerState::Working);
    EXPECT_EQ(f.registry.get<world::Builder>(worker)->site, site);
    const auto* follower = f.registry.get<world::RouteFollower>(worker);
    ASSERT_NE(follower, nullptr);
    EXPECT_EQ(follower->route, (world::RouteKey{hq, site}));
    EXPECT_TRUE(follower->forward);
}

TEST(WorkerSystem, WorkersBeyondTheSiteLimitStayIdle) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    f.add_site(SiteTile, 20.0f); // room for 2
    for (int i = 0; i < 3; ++i) {
        f.add_worker({0.5f, 0.5f});
    }

    // When
    f.system.update(Frame);
    f.system.update(Frame);

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Working), 2);
    EXPECT_EQ(f.workers_in(world::WorkerState::Idle), 1);
}

TEST(WorkerSystem, WithoutHeadquartersWorkersStayIdle) {
    // Given
    Fixture f;
    f.add_site(SiteTile, 300.0f);
    f.add_worker({0.5f, 0.5f});

    // When
    f.system.update(Frame);

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Working), 0);
}

TEST(WorkerSystem, AWorkerWhoseSiteIsFinishedTurnsIdleAndWalksBack) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto site = f.add_site(SiteTile, 300.0f);
    const auto worker = f.add_worker({4.5f, 0.5f});
    f.system.update(Frame);

    // When
    f.registry.remove<world::ConstructionSite>(site);
    f.system.update(Frame);

    // Then
    EXPECT_EQ(f.registry.get<world::Worker>(worker)->state, world::WorkerState::Idle);
    EXPECT_EQ(f.registry.get<world::Builder>(worker)->site, 0u);
    const auto* follower = f.registry.get<world::RouteFollower>(worker);
    ASSERT_NE(follower, nullptr);
    EXPECT_FALSE(follower->forward);
}

TEST(WorkerSystem, AWorkerWalksToTheSiteBuildsItAndComesBack) {
    // Given
    Fixture f;
    world::RouteNetwork network{std::make_unique<world::HeadquartersRoutePlanner>()};
    world::RouteFollowSystem follow{f.registry, network};
    world::CharacterMoveSystem move{f.registry, f.bus};
    world::ConstructionWorkSystem work{f.registry, f.bus};
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto site = f.add_site(SiteTile, 10.0f);
    const auto worker = f.add_worker({0.5f, 0.5f});
    network.rebuild(f.registry, f.terrain);

    // When: 4 tiles at 4 tiles/s to the site, 1 s of work, 4 tiles back
    for (int i = 0; i < 100; ++i) {
        f.system.update(Frame);
        follow.update(Frame);
        move.update(Frame);
        work.update(Frame);
    }

    // Then
    EXPECT_FALSE(f.registry.has<world::ConstructionSite>(site));
    EXPECT_EQ(f.registry.get<world::Worker>(worker)->state, world::WorkerState::Idle);
    const auto& at = f.registry.get<world::Position>(worker)->point;
    EXPECT_NEAR(at.x, 0.5f, 0.01f);
    EXPECT_NEAR(at.y, 0.5f, 0.01f);
}

TEST(WorkerSystem, IdleWorkersAreNotSentToProductionBuildingsByThemselves) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    f.add_workplace(SiteTile, 10);
    f.add_worker({0.5f, 0.5f});

    // When
    f.system.update(Frame);

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Idle), 1);
}

TEST(WorkerSystem, AnAssignmentSendsIdleWorkersToTheBuildingAlongTheRoute) {
    // Given
    Fixture f;
    const auto hq = f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto mine = f.add_workplace(SiteTile, 10);
    for (int i = 0; i < 5; ++i) {
        f.add_worker({0.5f, 0.5f});
    }

    // When
    f.bus.publish(world::WorkerAssignmentRequested{mine, 3});

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Working), 3);
    f.registry.each<world::Worker>([&](core::EntityId id, world::Worker& worker) {
        if (worker.state != world::WorkerState::Working) {
            return;
        }
        EXPECT_EQ(worker.workplace, mine);
        const auto* follower = f.registry.get<world::RouteFollower>(id);
        ASSERT_NE(follower, nullptr);
        EXPECT_EQ(follower->route, (world::RouteKey{hq, mine}));
    });
}

TEST(WorkerSystem, AnAssignmentNeverGoesBeyondTheWorkersTheBuildingNeeds) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto mine = f.add_workplace(SiteTile, 10);
    for (int i = 0; i < 15; ++i) {
        f.add_worker({0.5f, 0.5f});
    }

    // When
    f.bus.publish(world::WorkerAssignmentRequested{mine, 8});
    f.bus.publish(world::WorkerAssignmentRequested{mine, 8});

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Working), 10);
}

TEST(WorkerSystem, AnUnfinishedBuildingTakesNoAssignedWorkers) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto mine = f.add_workplace(SiteTile, 10);
    f.registry.add(mine, world::ConstructionSite{0.0f, 0.0f});
    f.add_worker({0.5f, 0.5f});

    // When
    f.bus.publish(world::WorkerAssignmentRequested{mine, 1});

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Working), 0);
}

TEST(WorkerSystem, ANegativeAssignmentSendsWorkersBackIdle) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto mine = f.add_workplace(SiteTile, 10);
    for (int i = 0; i < 4; ++i) {
        f.add_worker({0.5f, 0.5f});
    }
    f.bus.publish(world::WorkerAssignmentRequested{mine, 4});

    // When
    f.bus.publish(world::WorkerAssignmentRequested{mine, -3});

    // Then
    EXPECT_EQ(f.workers_in(world::WorkerState::Working), 1);
    int walking_back = 0;
    f.registry.each<world::Worker, world::RouteFollower>(
        [&](core::EntityId, world::Worker& worker, world::RouteFollower& follower) {
            if (worker.state == world::WorkerState::Idle) {
                EXPECT_EQ(worker.workplace, 0u);
                walking_back += follower.forward ? 0 : 1;
            }
        });
    EXPECT_EQ(walking_back, 3);
}

TEST(WorkerSystem, ProductionWorkersStayWhenAConstructionSiteAppears) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto mine = f.add_workplace(SiteTile, 10);
    const auto worker = f.add_worker({0.5f, 0.5f});
    f.bus.publish(world::WorkerAssignmentRequested{mine, 1});
    f.add_site({6, 0}, 300.0f);

    // When
    f.system.update(Frame);
    f.system.update(Frame);

    // Then
    EXPECT_EQ(f.registry.get<world::Worker>(worker)->workplace, mine);
    EXPECT_EQ(f.registry.get<world::Builder>(worker)->site, 0u);
}

TEST(WorkerReport, CountsIdleWorkersAndTheAssignedWorkersOfEachFinishedWorkplace) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto mine = f.add_workplace(SiteTile, 10);
    const auto unfinished = f.add_workplace({6, 0}, 10);
    f.registry.add(unfinished, world::ConstructionSite{0.0f, 300.0f});
    for (int i = 0; i < 5; ++i) {
        f.add_worker({0.5f, 0.5f});
    }
    f.bus.publish(world::WorkerAssignmentRequested{mine, 2});

    // When
    const world::WorkerReport report = world::worker_report(f.registry);

    // Then
    EXPECT_EQ(report.idle, 3);
    ASSERT_EQ(report.workplaces.size(), 1u);
    EXPECT_EQ(report.workplaces[0].id, mine);
    EXPECT_EQ(report.workplaces[0].assigned, 2);
    EXPECT_EQ(report.workplaces[0].needed, 10);
}

TEST(WorkerAtWork, AWorkerAtItsWorkplaceOrSiteIsAtWorkButNotWhileWalkingOrIdle) {
    // Given
    Fixture f;
    f.add_building(world::BuildingType::Headquarters, HqTile);
    const auto mine = f.add_workplace(SiteTile, 10);
    const auto site = f.add_site({8, 0}, 300.0f);
    const auto producing = f.add_worker({4.5f, 0.5f});
    const auto building = f.add_worker({8.5f, 0.5f});
    const auto walking = f.add_worker({0.5f, 0.5f});
    const auto idle = f.add_worker({4.5f, 0.5f});
    *f.registry.get<world::Worker>(producing) = {world::WorkerState::Working, mine};
    *f.registry.get<world::Worker>(walking) = {world::WorkerState::Working, mine};
    f.registry.get<world::Worker>(building)->state = world::WorkerState::Working;
    f.registry.get<world::Builder>(building)->site = site;

    // When / Then
    EXPECT_TRUE(world::is_at_work(f.registry, producing));
    EXPECT_TRUE(world::is_at_work(f.registry, building));
    EXPECT_FALSE(world::is_at_work(f.registry, walking));
    EXPECT_FALSE(world::is_at_work(f.registry, idle));
}
