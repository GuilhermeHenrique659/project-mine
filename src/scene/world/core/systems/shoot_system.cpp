#include "scene/world/core/systems/shoot_system.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <random>
#include <utility>

#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/events/combat_events.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

std::optional<WeaponStats> held_weapon(const Inventory& inventory) {
    for (const auto& slot : inventory.slots()) {
        if (!slot) {
            continue;
        }
        if (const auto stats = weapon_stats(slot->item.type)) {
            return stats;
        }
    }
    return std::nullopt;
}

HitRoll random_hit_roll() {
    auto generator = std::make_shared<std::mt19937>(std::random_device{}());
    return [generator] { return std::uniform_real_distribution<float>{0.0f, 1.0f}(*generator); };
}

float hit_chance(const WeaponStats& weapon, int experience) {
    return std::clamp(weapon.accuracy + experience * AccuracyPerExperience, 0.0f, 1.0f);
}

ShootSystem::ShootSystem(core::Registry& registry, event::EventBus& bus, HitRoll roll)
    : registry_(registry),
      bus_(bus),
      roll_(std::move(roll)),
      subscription_(event::subscribe_scoped<ShotRequested>(
          bus, [this](const ShotRequested& request) { shoot(request.shooter, request.target); })),
      attack_subscription_(event::subscribe_scoped<UnitsAttackRequested>(
          bus,
          [this](const UnitsAttackRequested& request) {
              for (const core::EntityId unit : request.units) {
                  if (registry_.has<Shooter>(unit) && unit != request.target) {
                      registry_.add(unit, AttackOrder{request.target});
                  }
              }
          })),
      move_subscription_(event::subscribe_scoped<UnitsMoveRequested>(
          bus, [this](const UnitsMoveRequested& request) {
              for (const core::EntityId unit : request.units) {
                  registry_.remove<AttackOrder>(unit);
              }
          })) {}

void ShootSystem::update(float delta_time) {
    registry_.each<Shooter>([&](core::EntityId, Shooter& shooter) {
        shooter.cooldown = std::max(0.0f, shooter.cooldown - delta_time);
    });
    registry_.each<AttackOrder>([&](core::EntityId id, AttackOrder& order) {
        const core::EntityId target = order.target;
        if (!registry_.is_alive(target)) {
            registry_.remove<AttackOrder>(id);
            return;
        }
        shoot(id, target);
        const auto* inventory = registry_.get<Inventory>(id);
        if (!registry_.is_alive(target) || inventory == nullptr ||
            inventory->count(items::Ammo) == 0) {
            registry_.remove<AttackOrder>(id);
        }
    });
}

void ShootSystem::shoot(core::EntityId shooter_id, core::EntityId target) {
    auto* shooter = registry_.get<Shooter>(shooter_id);
    auto* inventory = registry_.get<Inventory>(shooter_id);
    const auto* from = registry_.get<Position>(shooter_id);
    const auto* to = registry_.get<Position>(target);
    if (shooter == nullptr || inventory == nullptr || from == nullptr || to == nullptr ||
        shooter_id == target || shooter->cooldown > 0.0f || inventory->count(items::Ammo) == 0) {
        return;
    }
    const auto weapon = held_weapon(*inventory);
    if (!weapon) {
        return;
    }
    const float distance = std::hypot(to->point.x - from->point.x, to->point.y - from->point.y);
    if (distance > weapon->range) {
        return;
    }
    inventory->remove(items::Ammo, 1);
    shooter->cooldown = weapon->cooldown;
    const bool hit = roll_() < hit_chance(*weapon, shooter->experience);
    ++shooter->experience;
    bus_.publish(ShotFired{shooter_id, target, hit});
    auto* health = registry_.get<Health>(target);
    if (!hit || health == nullptr) {
        return;
    }
    health->current -= weapon->damage;
    if (health->current > 0) {
        return;
    }
    if (registry_.has<TrainingTarget>(target)) {
        health->current = MaxHealth;
        return;
    }
    registry_.destroy(target);
}

} // namespace world
