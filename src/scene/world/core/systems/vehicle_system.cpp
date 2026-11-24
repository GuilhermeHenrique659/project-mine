#include "scene/world/core/systems/vehicle_system.hpp"

#include <cmath>
#include <vector>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/entities/vehicle.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/events/vehicle_events.hpp"
#include "scene/world/core/spawn.hpp"

namespace world {

VehicleSystem::VehicleSystem(core::Registry& registry, event::EventBus& bus, const Terrain& terrain)
    : registry_(registry),
      bus_(bus),
      terrain_(terrain),
      board_subscription_(event::subscribe_scoped<VehicleBoardRequested>(
          bus,
          [this](const VehicleBoardRequested& request) {
              if (within_reach(request.vehicle, request.passenger)) {
                  board(registry_, request.vehicle, request.passenger);
              }
          })),
      exit_subscription_(event::subscribe_scoped<VehicleExitRequested>(
          bus, [this](const VehicleExitRequested& request) { exit(request.passenger); })),
      order_subscription_(event::subscribe_scoped<UnitsBoardRequested>(
          bus,
          [this](const UnitsBoardRequested& request) {
              if (!registry_.has<Vehicle>(request.vehicle)) {
                  return;
              }
              for (const core::EntityId unit : request.units) {
                  if (unit == request.vehicle || registry_.has<Vehicle>(unit) ||
                      registry_.has<Passenger>(unit) || !registry_.has<Position>(unit)) {
                      continue;
                  }
                  registry_.add(unit, BoardOrder{request.vehicle, std::nullopt});
              }
          })),
      move_subscription_(event::subscribe_scoped<UnitsMoveRequested>(
          bus, [this](const UnitsMoveRequested& request) {
              for (const core::EntityId unit : request.units) {
                  registry_.remove<BoardOrder>(unit);
              }
          })),
      unload_subscription_(event::subscribe_scoped<VehicleUnloadRequested>(
          bus, [this](const VehicleUnloadRequested& request) {
              const Vehicle* vehicle = registry_.get<Vehicle>(request.vehicle);
              if (vehicle == nullptr) {
                  return;
              }
              // A copy: each exit takes its passenger off the list.
              const std::vector<core::EntityId> passengers = vehicle->passengers;
              for (const core::EntityId passenger : passengers) {
                  if (!registry_.has<PlayerControlled>(passenger)) {
                      exit(passenger);
                  }
              }
          })) {}

bool VehicleSystem::within_reach(core::EntityId vehicle, core::EntityId passenger) const {
    const auto* from = registry_.get<Position>(passenger);
    const auto* to = registry_.get<Position>(vehicle);
    if (from == nullptr || to == nullptr) {
        return false;
    }
    return std::hypot(to->point.x - from->point.x, to->point.y - from->point.y) <= BoardReach;
}

void VehicleSystem::exit(core::EntityId passenger) {
    const Passenger* seat = registry_.get<Passenger>(passenger);
    if (seat == nullptr) {
        return;
    }
    const core::EntityId vehicle = seat->vehicle;
    leave(registry_, passenger);
    const auto* at = registry_.get<Position>(vehicle);
    auto* position = registry_.get<Position>(passenger);
    if (at == nullptr || position == nullptr) {
        return;
    }
    // The nearest free tile around the vehicle's own tile, as if it were a 1x1 building.
    const Building footprint{BuildingType::Headquarters, tile_containing(at->point)};
    const auto tile = spawn_tile(registry_, terrain_, footprint);
    position->point = tile ? WorldPoint{tile->x + 0.5f, tile->y + 0.5f} : at->point;
}

void VehicleSystem::update(float /*delta_time*/) {
    registry_.each<BoardOrder>([&](core::EntityId id, BoardOrder& order) {
        const core::EntityId vehicle = order.vehicle;
        const auto* at = registry_.get<Position>(vehicle);
        if (at == nullptr) {
            registry_.remove<BoardOrder>(id);
            return;
        }
        if (within_reach(vehicle, id)) {
            registry_.remove<BoardOrder>(id);
            registry_.remove<PathFollower>(id);
            board(registry_, vehicle, id);
            return;
        }
        const TileCoord goal = tile_containing(at->point);
        if (order.approach_goal == goal) {
            return;
        }
        order.approach_goal = goal;
        bus_.publish(UnitsApproachRequested{{id}, goal});
    });
    // Collected first: letting someone out changes the Passenger pool being walked.
    std::vector<core::EntityId> stranded;
    registry_.each<Passenger, Position>(
        [&](core::EntityId id, Passenger& seat, Position& position) {
            const auto* at = registry_.get<Position>(seat.vehicle);
            if (at == nullptr) {
                stranded.push_back(id);
                return;
            }
            position.point = at->point;
            if (Mover* mover = registry_.get<Mover>(id)) {
                mover->direction = {};
            }
        });
    for (const core::EntityId id : stranded) {
        registry_.remove<Passenger>(id);
    }
}

} // namespace world
