#include <gtest/gtest.h>

#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/entities/character.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/entities/worker.hpp"
#include "scene/world/core/events/combat_events.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/items/item.hpp"
#include "scene/world/core/systems/shoot_system.hpp"

namespace {

const world::WeaponStats Gun = *world::weapon_stats(world::ItemType::Weapon);

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    // What every shot rolls: 0 always hits, unless said otherwise.
    float roll = 0.0f;
    world::ShootSystem system{registry, bus, [this] { return roll; }};

    core::EntityId shooter(int weapons, int ammo, world::WorldPoint at = {0.0f, 0.0f}) {
        const core::EntityId id = registry.create("Shooter");
        registry.add(id, world::Position{at});
        registry.add(id, world::Shooter{});
        auto& inventory = registry.add(id, world::Inventory{2});
        inventory.add(world::items::Weapon, weapons);
        inventory.add(world::items::Ammo, ammo);
        return id;
    }

    core::EntityId target(float distance) {
        const core::EntityId id = registry.create("Target");
        registry.add(id, world::Position{{distance, 0.0f}});
        registry.add(id, world::Health{});
        return id;
    }

    void shoot(core::EntityId from, core::EntityId at) { bus.publish(world::ShotRequested{from, at}); }

    int ammo(core::EntityId id) { return registry.get<world::Inventory>(id)->count(world::items::Ammo); }

    int health(core::EntityId id) { return registry.get<world::Health>(id)->current; }
};

} // namespace

TEST(ShootSystem, ArmedShooterSpendsOneAmmoAndDamagesATargetInRange) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = f.target(Gun.range);
    int fired = 0;
    auto counter = event::subscribe_scoped<world::ShotFired>(f.bus, [&](const world::ShotFired&) { ++fired; });

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.ammo(shooter), 4);
    EXPECT_EQ(f.health(target), world::MaxHealth - Gun.damage);
    EXPECT_EQ(fired, 1);
}

TEST(ShootSystem, TargetBeyondTheWeaponRangeIsNotShot) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = f.target(Gun.range + 0.1f);

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.ammo(shooter), 5);
    EXPECT_EQ(f.health(target), world::MaxHealth);
}

TEST(ShootSystem, ShooterWithoutAWeaponCannotShoot) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(0, 5);
    const auto target = f.target(1.0f);

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.ammo(shooter), 5);
    EXPECT_EQ(f.health(target), world::MaxHealth);
}

TEST(ShootSystem, ShooterWithoutAmmoCannotShoot) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 0);
    const auto target = f.target(1.0f);

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.health(target), world::MaxHealth);
}

TEST(ShootSystem, ShooterCannotFireAgainUntilTheCooldownPasses) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = f.target(1.0f);
    f.shoot(shooter, target);

    // When
    f.shoot(shooter, target);
    const int ammo_during_cooldown = f.ammo(shooter);
    f.system.update(Gun.cooldown);
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(ammo_during_cooldown, 4);
    EXPECT_EQ(f.ammo(shooter), 3);
    EXPECT_EQ(f.health(target), world::MaxHealth - 2 * Gun.damage);
}

TEST(ShootSystem, TargetIsDestroyedWhenItsHealthReachesZero) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = f.target(1.0f);
    f.registry.get<world::Health>(target)->current = Gun.damage;

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_FALSE(f.registry.is_alive(target));
}

TEST(ShootSystem, SoldierAndPlayerCharacterCanShootEachOther) {
    // Given
    Fixture f;
    const auto soldier = world::spawn_soldier(f.registry, "Soldier", {0.0f, 0.0f});
    const auto player = world::spawn_character(f.registry, "Player", {1.0f, 0.0f});
    f.registry.add(player, world::Shooter{}); // the WorldModel arms the player
    for (const auto id : {soldier, player}) {
        f.registry.get<world::Inventory>(id)->add(world::items::Weapon, 1);
        f.registry.get<world::Inventory>(id)->add(world::items::Ammo, 3);
    }

    // When
    f.shoot(soldier, player);
    f.shoot(player, soldier);

    // Then
    EXPECT_EQ(f.health(player), world::MaxHealth - Gun.damage);
    EXPECT_EQ(f.health(soldier), world::MaxHealth - Gun.damage);
}

TEST(ShootSystem, BuiltTrainingTargetTakesShotsAndIsRestoredInsteadOfDestroyed) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = world::spawn_building(f.registry, "Training Target");
    world::make_building(f.registry, target,
                         *world::find_construction(world::BuildingType::TrainingTarget), {2, 0});
    f.registry.get<world::Health>(target)->current = Gun.damage;

    // When
    f.shoot(shooter, target);

    // Then
    ASSERT_TRUE(f.registry.is_alive(target));
    EXPECT_EQ(f.health(target), world::MaxHealth);
    EXPECT_EQ(f.ammo(shooter), 4);
}

TEST(ShootSystem, AnAttackOrderKeepsFiringUntilTheTargetIsDestroyed) {
    // Given: 4 hits kill a target
    Fixture f;
    const auto shooter = f.shooter(1, 10);
    const auto target = f.target(1.0f);
    f.bus.publish(world::UnitsAttackRequested{{shooter}, target});

    // When: 10 seconds pass
    for (int step = 0; step < 100; ++step) {
        f.system.update(0.1f);
    }

    // Then
    EXPECT_FALSE(f.registry.is_alive(target));
    EXPECT_EQ(f.ammo(shooter), 10 - world::MaxHealth / Gun.damage);
    EXPECT_FALSE(f.registry.has<world::AttackOrder>(shooter));
}

TEST(ShootSystem, AMoveOrderCancelsTheAttack) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 10);
    const auto target = f.target(1.0f);
    f.bus.publish(world::UnitsAttackRequested{{shooter}, target});
    f.system.update(0.1f);

    // When
    f.bus.publish(world::UnitsMoveRequested{{shooter}, {5, 5}});
    for (int step = 0; step < 50; ++step) {
        f.system.update(0.1f);
    }

    // Then: only the shot before the move
    EXPECT_EQ(f.health(target), world::MaxHealth - Gun.damage);
    EXPECT_FALSE(f.registry.has<world::AttackOrder>(shooter));
}

TEST(ShootSystem, RunningOutOfAmmoEndsTheAttack) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 1);
    const auto target = f.target(1.0f);
    f.bus.publish(world::UnitsAttackRequested{{shooter}, target});

    // When
    f.system.update(0.1f);

    // Then
    EXPECT_EQ(f.health(target), world::MaxHealth - Gun.damage);
    EXPECT_FALSE(f.registry.has<world::AttackOrder>(shooter));
}

TEST(ShootSystem, AWorkerCanBeShotButCannotShootEvenWhenArmed) {
    // Given
    Fixture f;
    const auto worker = world::spawn_worker(f.registry, "Worker", {0.0f, 0.0f});
    f.registry.add(worker, world::Inventory{2}); // room for a weapon and its ammo
    f.registry.get<world::Inventory>(worker)->add(world::items::Weapon, 1);
    f.registry.get<world::Inventory>(worker)->add(world::items::Ammo, 5);
    const auto soldier = world::spawn_soldier(f.registry, "Soldier", {1.0f, 0.0f});
    f.registry.get<world::Inventory>(soldier)->add(world::items::Weapon, 1);
    f.registry.get<world::Inventory>(soldier)->add(world::items::Ammo, 5);

    // When
    f.shoot(worker, soldier);
    f.shoot(soldier, worker);

    // Then
    EXPECT_EQ(f.health(soldier), world::MaxHealth);
    EXPECT_EQ(f.health(worker), world::MaxHealth - Gun.damage);
}

TEST(HitChance, AShooterWithoutExperienceHitsWithTheWeaponAccuracy) {
    // Given / When
    const float chance = world::hit_chance(Gun, 0);

    // Then
    EXPECT_FLOAT_EQ(chance, 0.6f);
}

TEST(HitChance, EachPointOfExperienceAddsHalfAPercent) {
    // Given / When
    const float chance = world::hit_chance(Gun, 40);

    // Then
    EXPECT_FLOAT_EQ(chance, 0.8f);
}

TEST(HitChance, ExperienceNeverTakesTheChanceAbove100Percent) {
    // Given / When
    const float chance = world::hit_chance(Gun, 1000);

    // Then
    EXPECT_FLOAT_EQ(chance, 1.0f);
}

TEST(HitChance, TheChanceNeverGoesBelowZero) {
    // Given
    world::WeaponStats wild = Gun;
    wild.accuracy = -0.5f;

    // When
    const float chance = world::hit_chance(wild, 0);

    // Then
    EXPECT_FLOAT_EQ(chance, 0.0f);
}

TEST(ShootSystem, ARollAboveTheHitChanceMissesButStillSpendsTheAmmo) {
    // Given: a 60% weapon without experience, rolling 0.7
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = f.target(1.0f);
    f.roll = 0.7f;
    std::vector<world::ShotFired> fired;
    auto counter = event::subscribe_scoped<world::ShotFired>(
        f.bus, [&](const world::ShotFired& shot) { fired.push_back(shot); });

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.health(target), world::MaxHealth);
    EXPECT_EQ(f.ammo(shooter), 4);
    ASSERT_EQ(fired.size(), 1u);
    EXPECT_FALSE(fired[0].hit);
}

TEST(ShootSystem, ExperienceTurnsTheSameRollIntoAHit) {
    // Given: rolling 0.7 with 40 experience (80% chance)
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    f.registry.get<world::Shooter>(shooter)->experience = 40;
    const auto target = f.target(1.0f);
    f.roll = 0.7f;

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.health(target), world::MaxHealth - Gun.damage);
}

TEST(ShootSystem, EveryShotFiredHitOrMissGivesOneExperience) {
    // Given
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = f.target(1.0f);

    // When: a hit, then a miss after the cooldown
    f.shoot(shooter, target);
    f.system.update(Gun.cooldown);
    f.roll = 0.99f;
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.registry.get<world::Shooter>(shooter)->experience, 2);
}

TEST(ShootSystem, AShotNotFiredGivesNoExperience) {
    // Given: a target out of range
    Fixture f;
    const auto shooter = f.shooter(1, 5);
    const auto target = f.target(Gun.range + 1.0f);

    // When
    f.shoot(shooter, target);

    // Then
    EXPECT_EQ(f.registry.get<world::Shooter>(shooter)->experience, 0);
}
