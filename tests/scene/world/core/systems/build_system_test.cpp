#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/persistent.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/construction/build_rules.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/core/systems/build_system.hpp"

namespace {

constexpr world::TileCoord Grass{2, 3}; // inside the spawn clearing: always grass

// Needs a headquarters, costs 10 iron ore and takes 4 units of work. Only one building
// type exists yet, so it borrows it.
world::Construction mine() {
    return {"Test Mine", "icon_test", world::BuildingType::Headquarters,
            {{world::items::IronOre, 10}}, 4.0f, {std::make_shared<world::NeedsHeadquartersRule>()}};
}

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::EntityFactory factory = world::make_entity_factory();
    core::EntityId player = registry.create("Player");

    Fixture() { registry.add(player, world::SolidInventory{10}); }

    std::size_t building_count() const {
        std::size_t count = 0;
        for (const core::Entity& entity : registry.entities()) {
            count += registry.has<world::Building>(entity.id) ? 1 : 0;
        }
        return count;
    }
};

} // namespace

TEST(BuildSystem, ARequestedHeadquartersStandsFinishedOnItsTile) {
    // Given
    Fixture f;
    world::BuildSystem system(f.registry, f.bus, f.terrain, f.factory, f.player);

    // When
    f.bus.publish(world::BuildRequested{world::BuildingType::Headquarters, Grass});

    // Then
    ASSERT_EQ(f.building_count(), 1u);
    const core::Entity& hq = f.registry.entities().back();
    EXPECT_EQ(hq.label, "Headquarters");
    EXPECT_EQ(f.registry.get<world::Building>(hq.id)->tile, Grass);
    EXPECT_FALSE(f.registry.has<world::ConstructionSite>(hq.id));
    EXPECT_EQ(f.registry.get<world::Persistent>(hq.id)->archetype, "building");
    const world::Position* position = f.registry.get<world::Position>(hq.id);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->point.x, 4.5f); // centre of its 5x5 footprint
    EXPECT_FLOAT_EQ(position->point.y, 5.5f);
}

TEST(BuildSystem, ARefusedRequestBuildsNothing) {
    // Given
    Fixture f;
    world::BuildSystem system(f.registry, f.bus, f.terrain, f.factory, f.player);
    f.bus.publish(world::BuildRequested{world::BuildingType::Headquarters, Grass});

    // When
    f.bus.publish(world::BuildRequested{world::BuildingType::Headquarters, {4, 4}});

    // Then
    EXPECT_EQ(f.building_count(), 1u);
}

TEST(BuildSystem, APlacedBuildingIsAnnouncedWithItsTile) {
    // Given
    Fixture f;
    world::BuildSystem system(f.registry, f.bus, f.terrain, f.factory, f.player);
    std::vector<world::TileCoord> placed;
    f.bus.subscribe<world::BuildingPlaced>(
        [&](const world::BuildingPlaced& event) { placed.push_back(event.tile); });

    // When
    f.bus.publish(world::BuildRequested{world::BuildingType::Headquarters, Grass});

    // Then
    ASSERT_EQ(placed.size(), 1u);
    EXPECT_EQ(placed.front(), Grass);
}

TEST(BuildSystem, ARefusedRequestAnnouncesNothing) {
    // Given: a headquarters already stands, so a second one is refused
    Fixture f;
    world::BuildSystem system(f.registry, f.bus, f.terrain, f.factory, f.player);
    f.bus.publish(world::BuildRequested{world::BuildingType::Headquarters, Grass});
    int placed = 0;
    f.bus.subscribe<world::BuildingPlaced>([&](const world::BuildingPlaced&) { ++placed; });

    // When
    f.bus.publish(world::BuildRequested{world::BuildingType::Headquarters, {4, 4}});

    // Then
    EXPECT_EQ(placed, 0);
}

TEST(Build, PaysTheCostAndStartsAConstructionSite) {
    // Given
    Fixture f;
    world::build(f.registry, f.factory, *world::find_construction(world::BuildingType::Headquarters),
                 {-3, -3}, f.terrain, f.player); // 5x5: clear of Grass
    world::SolidInventory* inventory = f.registry.get<world::SolidInventory>(f.player);
    inventory->add(world::items::IronOre, 15);

    // When
    const auto result = world::build(f.registry, f.factory, mine(), Grass, f.terrain, f.player);

    // Then
    EXPECT_EQ(result, world::BuildResult::Ok);
    EXPECT_EQ(inventory->count(world::items::IronOre), 5);
    const world::ConstructionSite* site =
        f.registry.get<world::ConstructionSite>(f.registry.entities().back().id);
    ASSERT_NE(site, nullptr);
    EXPECT_FLOAT_EQ(site->progress, 0.0f);
    EXPECT_FLOAT_EQ(site->effort, 4.0f);
}

TEST(Build, WithoutEnoughResourcesTakesNothing) {
    // Given
    Fixture f;
    world::build(f.registry, f.factory, *world::find_construction(world::BuildingType::Headquarters),
                 {-3, -3}, f.terrain, f.player); // 5x5: clear of Grass
    world::SolidInventory* inventory = f.registry.get<world::SolidInventory>(f.player);
    inventory->add(world::items::IronOre, 9);

    // When
    const auto result = world::build(f.registry, f.factory, mine(), Grass, f.terrain, f.player);

    // Then
    EXPECT_EQ(result, world::BuildResult::NotEnoughResources);
    EXPECT_EQ(inventory->count(world::items::IronOre), 9);
    EXPECT_EQ(f.building_count(), 1u);
}

namespace {

// A site at world (5.5, 5.5) needing 30 units and a builder working on it from `at`.
struct WorkFixture : Fixture {
    core::EntityId site = registry.create("Mine");
    core::EntityId builder = registry.create("Builder");
    world::ConstructionWorkSystem system{registry, bus};
    int completed = 0;
    event::ScopedSubscription subscription = event::subscribe_scoped<world::BuildingCompleted>(
        bus, [this](const world::BuildingCompleted&) { ++completed; });

    explicit WorkFixture(world::WorldPoint at) {
        registry.add(site, world::Building{world::BuildingType::Mine, {5, 5}});
        registry.add(site, world::Position{{5.5f, 5.5f}});
        registry.add(site, world::ConstructionSite{0.0f, 30.0f});
        registry.add(builder, world::Position{at});
        registry.add(builder, world::Builder{10.0f, site});
    }

    float progress() const { return registry.get<world::ConstructionSite>(site)->progress; }
};

} // namespace

TEST(ConstructionWorkSystem, ABuilderInReachAdvancesTheSiteByItsRate) {
    // Given
    WorkFixture f({6.5f, 5.5f}); // one tile away

    // When
    f.system.update(0.5f);

    // Then
    EXPECT_FLOAT_EQ(f.progress(), 5.0f);
}

TEST(ConstructionWorkSystem, ASiteWithNobodyWorkingOnItDoesNotAdvance) {
    // Given
    WorkFixture f({6.5f, 5.5f});
    f.registry.get<world::Builder>(f.builder)->site = 0;

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(f.progress(), 0.0f);
}

TEST(ConstructionWorkSystem, ABuilderOutOfReachDoesNotAdvanceTheSite) {
    // Given
    WorkFixture f({6.0f + world::BuildReach + 0.1f, 5.5f}); // reach counts from the edge (x 6)

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(f.progress(), 0.0f);
}

TEST(ConstructionWorkSystem, FinishesTheSiteAtItsEffortAndAnnouncesItOnce) {
    // Given
    WorkFixture f({6.5f, 5.5f});

    // When
    f.system.update(2.0f);
    f.system.update(1.0f);
    f.system.update(1.0f);

    // Then
    EXPECT_FALSE(f.registry.has<world::ConstructionSite>(f.site));
    EXPECT_EQ(f.completed, 1);
    EXPECT_EQ(f.registry.get<world::Builder>(f.builder)->site, 0u);
}

TEST(MakeBuilding, AMineHasAnInventoryAndTheHeadquartersNone) {
    // Given
    core::Registry registry;
    const core::EntityId mine = registry.create("Mine");
    const core::EntityId hq = registry.create("Headquarters");

    // When
    world::make_building(registry, mine, *world::find_construction(world::BuildingType::Mine), {1, 1});
    world::make_building(registry, hq,
                         *world::find_construction(world::BuildingType::Headquarters), {2, 2});

    // Then
    const world::SolidInventory* inventory = registry.get<world::SolidInventory>(mine);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->capacity(), 10u);
    EXPECT_FALSE(registry.has<world::SolidInventory>(hq));
}
