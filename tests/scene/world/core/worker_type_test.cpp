#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/worker_type.hpp"

namespace {

struct Fixture {
    core::Registry registry;
    core::EntityId worker = world::spawn_worker(registry, "Worker", {0.5f, 0.5f});

    core::EntityId add_building(world::BuildingType type) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, {4, 0}});
        return id;
    }

    world::WorkerType type() const { return world::worker_type(registry, worker); }
};

} // namespace

TEST(Unit, AWorkerIsAUnitAndThePlayersCharacterIsNot) {
    // Given
    core::Registry registry;

    // When
    const auto worker = world::spawn_worker(registry, "Worker", {0.0f, 0.0f});
    const auto player = world::spawn_character(registry, "Player", {0.0f, 0.0f});

    // Then
    EXPECT_TRUE(registry.has<world::Unit>(worker));
    EXPECT_FALSE(registry.has<world::Unit>(player));
}

TEST(WorkerType, AnIdleWorkerHasNoType) {
    // Given / When
    Fixture f;

    // Then
    EXPECT_EQ(f.type(), world::WorkerType::None);
    EXPECT_STREQ(world::to_string(f.type()), "Idle");
}

TEST(WorkerType, AWorkerAssignedToAMineIsAMiner) {
    // Given
    Fixture f;
    const auto mine = f.add_building(world::BuildingType::Mine);

    // When
    *f.registry.get<world::Worker>(f.worker) = {world::WorkerState::Working, mine};

    // Then
    EXPECT_EQ(f.type(), world::WorkerType::Miner);
    EXPECT_STREQ(world::to_string(f.type()), "Miner");
}

TEST(WorkerType, AWorkerOnAConstructionSiteIsABuilder) {
    // Given
    Fixture f;
    const auto site = f.add_building(world::BuildingType::Storage);
    f.registry.add(site, world::ConstructionSite{0.0f, 600.0f});

    // When
    f.registry.get<world::Worker>(f.worker)->state = world::WorkerState::Working;
    f.registry.get<world::Builder>(f.worker)->site = site;

    // Then
    EXPECT_EQ(f.type(), world::WorkerType::Builder);
}

TEST(WorkerType, AWorkerOnALogisticRouteIsAHauler) {
    // Given
    Fixture f;
    const auto storage = f.add_building(world::BuildingType::Storage);
    const auto mine = f.add_building(world::BuildingType::Mine);

    // When
    f.registry.get<world::Worker>(f.worker)->state = world::WorkerState::Working;
    f.registry.add(f.worker, world::Hauler{{storage, mine}});

    // Then
    EXPECT_EQ(f.type(), world::WorkerType::Hauler);
}

TEST(WorkerType, AWorkerGoingHomeFromItsSiteHasNoTypeAnyMore) {
    // Given: idle again, the site link already cleared
    Fixture f;
    const auto mine = f.add_building(world::BuildingType::Mine);
    *f.registry.get<world::Worker>(f.worker) = {world::WorkerState::Working, mine};

    // When
    *f.registry.get<world::Worker>(f.worker) = {world::WorkerState::Idle, 0};

    // Then
    EXPECT_EQ(f.type(), world::WorkerType::None);
}
