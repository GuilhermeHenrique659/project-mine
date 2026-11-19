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
        barracks = registry.create("building");
        world::make_building(registry, barracks,
                             *world::find_construction(world::BuildingType::Barracks), {2, 0});
        registry.remove<world::ConstructionSite>(barracks);
        supply().add(world::items::Weapon, 3);
        supply().add(world::items::Ammo, 300);
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
