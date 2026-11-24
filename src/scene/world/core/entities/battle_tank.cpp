#include "scene/world/core/entities/battle_tank.hpp"

#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/entities/vehicle.hpp"

namespace world {

core::EntityId spawn_battle_tank(core::Registry& registry, std::string label, WorldPoint position) {
    const core::EntityId id =
        spawn_vehicle(registry, std::move(label), position,
                      {BattleTankSeats, BattleTankSpeed, BattleTankHealth, BattleTankInventorySlots});
    registry.add(id, BattleTank{});
    registry.add(id, Shooter{});
    registry.add(id, MountedWeapon{TankCannon});
    return id;
}

} // namespace world
