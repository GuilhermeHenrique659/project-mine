#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/systems/resupply_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

// A finished barracks covering tiles (2..4, 0..2) holding 3 weapons and 300 ammo; the spawn
// clearing around the origin is dry land.
struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::Terrain terrain{7};
    world::ResupplySystem system{registry, bus, terrain};
    core::EntityId barracks = 0;

    Fixture() {
        world::track_buildings(registry);
        barracks = add_barracks({2, 0});
        supply().add(world::items::Weapon, 3);
        supply().add(world::items::Ammo, 300);
    }

    // A finished, empty barracks on `tile`.
    core::EntityId add_barracks(world::TileCoord tile) {
        const core::EntityId id = registry.create("building");
        world::make_building(registry, id,
                             *world::find_construction(world::BuildingType::Barracks), tile);
        registry.remove<world::ConstructionSite>(id);
        return id;
    }

    const world::ResupplyOrder* order(core::EntityId id) const {
        return registry.get<world::ResupplyOrder>(id);
    }

    world::Inventory& supply() { return *registry.get<world::Inventory>(barracks); }
    world::Inventory& carried(core::EntityId id) { return *registry.get<world::Inventory>(id); }
};

} // namespace

TEST(ResupplySystem, ASoldierNextToTheSupplyTakesOneWeaponAndOneAmmoStack) {
    // Given
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {1.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsResupplyRequested{{soldier}, f.barracks});
    f.system.update(0.1f);

    // Then
    EXPECT_EQ(f.carried(soldier).count(world::items::Weapon), 1);
    EXPECT_EQ(f.carried(soldier).count(world::items::Ammo), 100);
    EXPECT_EQ(f.supply().count(world::items::Weapon), 2);
    EXPECT_EQ(f.supply().count(world::items::Ammo), 200);
    EXPECT_FALSE(f.registry.has<world::ResupplyOrder>(soldier));
}

TEST(ResupplySystem, AnArmedSoldierKeepsItsWeaponAndOnlyTopsUpItsAmmo) {
    // Given: a soldier carrying a weapon and 40 ammo
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {1.5f, 0.5f});
    f.carried(soldier).add(world::items::Weapon, 1);
    f.carried(soldier).add(world::items::Ammo, 40);

    // When
    f.bus.publish(world::UnitsResupplyRequested{{soldier}, f.barracks});
    f.system.update(0.1f);

    // Then
    EXPECT_EQ(f.carried(soldier).count(world::items::Weapon), 1);
    EXPECT_EQ(f.carried(soldier).count(world::items::Ammo), 100);
    EXPECT_EQ(f.supply().count(world::items::Weapon), 3);
    EXPECT_EQ(f.supply().count(world::items::Ammo), 240);
}

TEST(ResupplySystem, ASoldierTakesWhatIsLeftWhenTheSupplyIsShort) {
    // Given: a supply with no weapon and 30 ammo
    Fixture f;
    f.supply().remove(world::items::Weapon, 3);
    f.supply().remove(world::items::Ammo, 270);
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {1.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsResupplyRequested{{soldier}, f.barracks});
    f.system.update(0.1f);

    // Then
    EXPECT_EQ(f.carried(soldier).count(world::items::Weapon), 0);
    EXPECT_EQ(f.carried(soldier).count(world::items::Ammo), 30);
    EXPECT_EQ(f.supply().count(world::items::Ammo), 0);
}

TEST(ResupplySystem, AFarSoldierWalksNextToTheSupplyBeforeTakingAnything) {
    // Given
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {-3.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsResupplyRequested{{soldier}, f.barracks});
    f.system.update(0.1f);

    // Then
    const auto* path = f.registry.get<world::PathFollower>(soldier);
    ASSERT_NE(path, nullptr);
    const world::TileCoord goal = path->tiles.back();
    EXPECT_LE(world::distance_to_building(f.registry, f.barracks,
                                          {goal.x + 0.5f, goal.y + 0.5f}),
              world::SupplyReach);
    EXPECT_EQ(world::building_at(f.registry, goal), 0u);
    EXPECT_TRUE(f.carried(soldier).is_empty());
    EXPECT_TRUE(f.registry.has<world::ResupplyOrder>(soldier));
}

TEST(ResupplySystem, AMoveOrderCancelsTheResupply) {
    // Given
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {-3.5f, 0.5f});
    f.bus.publish(world::UnitsResupplyRequested{{soldier}, f.barracks});

    // When
    f.bus.publish(world::UnitsMoveRequested{{soldier}, {-6, 0}});

    // Then
    EXPECT_FALSE(f.registry.has<world::ResupplyOrder>(soldier));
}

TEST(ResupplySystem, NothingIsTakenFromABarracksStillUnderConstruction) {
    // Given
    Fixture f;
    f.registry.add(f.barracks, world::ConstructionSite{});
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {1.5f, 0.5f});

    // When
    f.bus.publish(world::UnitsResupplyRequested{{soldier}, f.barracks});
    f.system.update(0.1f);

    // Then
    EXPECT_TRUE(f.carried(soldier).is_empty());
    EXPECT_FALSE(f.registry.has<world::ResupplyOrder>(soldier));
}

TEST(ResupplySystem, EveryFiveSecondsAnUnarmedSoldierSeeksTheNearestSupply) {
    // Given: a second stocked barracks further away, on (-10, 0)
    Fixture f;
    const core::EntityId far = f.add_barracks({-10, 0});
    f.registry.get<world::Inventory>(far)->add(world::items::Weapon, 1);
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {-1.5f, 4.5f});

    // When
    f.system.update(world::AutoResupplyInterval);

    // Then
    ASSERT_NE(f.order(soldier), nullptr);
    EXPECT_EQ(f.order(soldier)->supply, f.barracks);
    EXPECT_NE(f.registry.get<world::PathFollower>(soldier), nullptr);
}

TEST(ResupplySystem, NoSoldierSeeksASupplyBeforeFiveSecondsPass) {
    // Given
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {-1.5f, 4.5f});

    // When
    f.system.update(world::AutoResupplyInterval - 0.5f);

    // Then
    EXPECT_EQ(f.order(soldier), nullptr);
}

TEST(ResupplySystem, ASoldierOutOfAmmoSeeksASupplyButAnArmedOneWithAmmoDoesNot) {
    // Given
    Fixture f;
    const core::EntityId empty = world::spawn_soldier(f.registry, "Soldier", {-1.5f, 4.5f});
    f.carried(empty).add(world::items::Weapon, 1);
    const core::EntityId ready = world::spawn_soldier(f.registry, "Soldier", {-1.5f, 5.5f});
    f.carried(ready).add(world::items::Weapon, 1);
    f.carried(ready).add(world::items::Ammo, 10);

    // When
    f.system.update(world::AutoResupplyInterval);

    // Then
    EXPECT_NE(f.order(empty), nullptr);
    EXPECT_EQ(f.order(ready), nullptr);
}

TEST(ResupplySystem, ASupplyWithoutWhatTheSoldierLacksIsSkipped) {
    // Given: the near barracks has no ammo left, a far one on (-10, 0) has some
    Fixture f;
    f.supply().remove(world::items::Ammo, 300);
    const core::EntityId far = f.add_barracks({-10, 0});
    f.registry.get<world::Inventory>(far)->add(world::items::Ammo, 50);
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {-1.5f, 4.5f});
    f.carried(soldier).add(world::items::Weapon, 1);

    // When
    f.system.update(world::AutoResupplyInterval);

    // Then
    ASSERT_NE(f.order(soldier), nullptr);
    EXPECT_EQ(f.order(soldier)->supply, far);
}

TEST(ResupplySystem, ASoldierWalkingSomewhereIsNotSentToResupply) {
    // Given: an unarmed soldier on a player's move order
    Fixture f;
    const core::EntityId soldier = world::spawn_soldier(f.registry, "Soldier", {-1.5f, 4.5f});
    f.registry.add(soldier, world::PathFollower{{{-1, 4}, {-1, 8}}});

    // When
    f.system.update(world::AutoResupplyInterval);

    // Then
    EXPECT_EQ(f.order(soldier), nullptr);
}
