#include "scene/world/core/entities/vehicle.hpp"

#include <algorithm>

#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/selected.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/entities/unit.hpp"

namespace world {

core::EntityId spawn_vehicle(core::Registry& registry, std::string label, WorldPoint position,
                             const VehicleSpec& spec) {
    const core::EntityId id = spawn_unit(registry, std::move(label), position);
    // A vehicle carries people, it does not build.
    registry.remove<Builder>(id);
    registry.add(id, Vehicle{spec.seats, {}});
    registry.add(id, Mover{spec.speed});
    registry.add(id, Health{spec.health});
    registry.add(id, SolidInventory{spec.inventory_slots});
    return id;
}

bool board(core::Registry& registry, core::EntityId vehicle_id, core::EntityId passenger) {
    Vehicle* vehicle = registry.get<Vehicle>(vehicle_id);
    if (vehicle == nullptr || vehicle_id == passenger || registry.has<Vehicle>(passenger) ||
        registry.has<Passenger>(passenger) || vehicle->passengers.size() >= vehicle->seats) {
        return false;
    }
    vehicle->passengers.push_back(passenger);
    registry.add(passenger, Passenger{vehicle_id});
    // Whatever it was doing on foot is over.
    registry.remove<PathFollower>(passenger);
    registry.remove<AttackOrder>(passenger);
    registry.remove<ResupplyOrder>(passenger);
    registry.remove<BoardOrder>(passenger);
    registry.remove<Selected>(passenger);
    if (Mover* mover = registry.get<Mover>(passenger)) {
        mover->direction = {};
    }
    return true;
}

void leave(core::Registry& registry, core::EntityId passenger) {
    const Passenger* seat = registry.get<Passenger>(passenger);
    if (seat == nullptr) {
        return;
    }
    if (Vehicle* vehicle = registry.get<Vehicle>(seat->vehicle)) {
        std::erase(vehicle->passengers, passenger);
    }
    registry.remove<Passenger>(passenger);
}

bool is_crewed(const core::Registry& registry, core::EntityId vehicle) {
    const Vehicle* found = registry.get<Vehicle>(vehicle);
    return found != nullptr && !found->passengers.empty();
}

bool carries_player(const core::Registry& registry, core::EntityId vehicle) {
    const Vehicle* found = registry.get<Vehicle>(vehicle);
    if (found == nullptr) {
        return false;
    }
    return std::any_of(found->passengers.begin(), found->passengers.end(),
                       [&](core::EntityId id) { return registry.has<PlayerControlled>(id); });
}

core::EntityId vehicle_at(const core::Registry& registry, TileCoord tile) {
    for (const core::Entity& entity : registry.entities()) {
        const auto* position = registry.get<Position>(entity.id);
        if (position != nullptr && registry.has<Vehicle>(entity.id) &&
            tile_containing(position->point) == tile) {
            return entity.id;
        }
    }
    return 0;
}

} // namespace world
