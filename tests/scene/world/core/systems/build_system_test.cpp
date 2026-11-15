#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
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
            {{world::items::IronOre, 10}}, 4.0f, false, true};
}

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::EntityFactory factory = world::make_entity_factory();
    core::EntityId player = registry.create("Player");

    Fixture() { registry.add(player, world::Inventory{10}); }

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
    EXPECT_FLOAT_EQ(position->point.x, 2.5f);
    EXPECT_FLOAT_EQ(position->point.y, 3.5f);
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

TEST(Build, PaysTheCostAndStartsAConstructionSite) {
    // Given
    Fixture f;
    world::build(f.registry, f.factory, *world::find_construction(world::BuildingType::Headquarters),
                 {0, 0}, f.terrain, f.player);
    world::Inventory* inventory = f.registry.get<world::Inventory>(f.player);
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
                 {0, 0}, f.terrain, f.player);
    world::Inventory* inventory = f.registry.get<world::Inventory>(f.player);
    inventory->add(world::items::IronOre, 9);

    // When
    const auto result = world::build(f.registry, f.factory, mine(), Grass, f.terrain, f.player);

    // Then
    EXPECT_EQ(result, world::BuildResult::NotEnoughResources);
    EXPECT_EQ(inventory->count(world::items::IronOre), 9);
    EXPECT_EQ(f.building_count(), 1u);
}

TEST(ConstructionProgressSystem, FinishesTheSiteAtItsEffortAndAnnouncesItOnce) {
    // Given
    Fixture f;
    const core::EntityId id = f.registry.create("site");
    f.registry.add(id, world::ConstructionSite{0.0f, 4.0f});
    world::ConstructionProgressSystem system(f.registry, f.bus);
    int completed = 0;
    core::EntityId completed_id = 0;
    const auto subscription = event::subscribe_scoped<world::BuildingCompleted>(
        f.bus, [&](const world::BuildingCompleted& event) {
            ++completed;
            completed_id = event.entity;
        });

    // When
    system.update(3.0f);
    const float progress = f.registry.get<world::ConstructionSite>(id)->progress;
    system.update(1.0f);
    system.update(1.0f);

    // Then
    EXPECT_FLOAT_EQ(progress, 3.0f);
    EXPECT_FALSE(f.registry.has<world::ConstructionSite>(id));
    EXPECT_EQ(completed, 1);
    EXPECT_EQ(completed_id, id);
}
