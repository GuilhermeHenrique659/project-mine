#include "scene/world/core/systems/haul_system.hpp"

#include <algorithm>
#include <vector>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/route_follower.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/events/worker_events.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

namespace {

core::EntityId find_headquarters(const core::Registry& registry) {
    for (const core::Entity& entity : registry.entities()) {
        const auto* building = registry.get<Building>(entity.id);
        if (building != nullptr && building->type == BuildingType::Headquarters &&
            !registry.has<ConstructionSite>(entity.id)) {
            return entity.id;
        }
    }
    return 0;
}

void follow(core::Registry& registry, core::EntityId id, RouteFollower follower) {
    if (auto* existing = registry.get<RouteFollower>(id)) {
        *existing = follower;
        return;
    }
    registry.add(id, follower);
}

// Fills the hauler's empty inventory with the first item in a general slot of `source`
// (filtered slots hold a building's inputs and stay), as much as fits.
void load(Inventory& hauler, Inventory& source) {
    if (!hauler.is_empty()) {
        return;
    }
    for (std::size_t i = 0; i < source.slots().size(); ++i) {
        const auto& slot = source.slots()[i];
        if (!slot || source.filter(i)) {
            continue;
        }
        const ItemStack stack = *slot;
        const int taken = stack.count - hauler.add(stack.item, stack.count);
        source.remove(stack.item, taken);
        return;
    }
}

// Moves everything the hauler carries into `destination`, as much as fits.
void unload(Inventory& hauler, Inventory& destination) {
    for (const auto& slot : hauler.slots()) {
        if (!slot) {
            continue;
        }
        const ItemStack stack = *slot;
        const int moved = stack.count - destination.add(stack.item, stack.count);
        hauler.remove(stack.item, moved);
    }
}

} // namespace

HaulSystem::HaulSystem(core::Registry& registry, event::EventBus& bus, const RouteNetwork& network)
    : registry_(registry),
      network_(network),
      haulers_subscription_(event::subscribe_scoped<HaulersRequested>(
          bus,
          [this](const HaulersRequested& request) {
              if (request.count > 0) {
                  assign(request.route, request.mode, request.count);
              } else {
                  release(request.route, -request.count);
              }
          })),
      mode_subscription_(event::subscribe_scoped<HaulModeRequested>(
          bus, [this](const HaulModeRequested& request) {
              registry_.each<Hauler>([&](core::EntityId, Hauler& hauler) {
                  if (hauler.route == request.route) {
                      hauler.mode = request.mode;
                  }
              });
          })) {}

void HaulSystem::assign(RouteKey route, HaulMode mode, int count) {
    const Route* found = network_.find(route);
    const core::EntityId headquarters = find_headquarters(registry_);
    if (found == nullptr || found->kind != RouteKind::Logistic || headquarters == 0) {
        return;
    }
    std::vector<core::EntityId> hired;
    registry_.each<Worker>([&](core::EntityId id, Worker& worker) {
        if (static_cast<int>(hired.size()) >= count || worker.state != WorkerState::Idle) {
            return;
        }
        worker.state = WorkerState::Working;
        worker.workplace = 0;
        hired.push_back(id);
    });
    for (const core::EntityId id : hired) {
        registry_.add(id, Hauler{route, mode});
        // First to the storage along the headquarters' route (joined where it stands).
        follow(registry_, id, RouteFollower{RouteKey{headquarters, route.from}});
    }
}

void HaulSystem::release(RouteKey route, int count) {
    std::vector<core::EntityId> released;
    registry_.each<Hauler>([&](core::EntityId id, Hauler& hauler) {
        if (static_cast<int>(released.size()) < count && hauler.route == route) {
            released.push_back(id);
        }
    });
    for (const core::EntityId id : released) {
        send_home(id);
    }
}

void HaulSystem::send_home(core::EntityId worker) {
    const Hauler* hauler = registry_.get<Hauler>(worker);
    if (hauler == nullptr) {
        return;
    }
    const core::EntityId storage = hauler->route.from;
    registry_.remove<Hauler>(worker);
    if (Worker* state = registry_.get<Worker>(worker)) {
        state->state = WorkerState::Idle;
        state->workplace = 0;
    }
    const core::EntityId headquarters = find_headquarters(registry_);
    if (headquarters == 0 || !registry_.has<Building>(storage)) {
        registry_.remove<RouteFollower>(worker);
        return;
    }
    // Backwards along headquarters -> storage, joined where it stands: home.
    RouteFollower home{RouteKey{headquarters, storage}};
    home.forward = false;
    follow(registry_, worker, home);
}

void HaulSystem::resolve_loaded() {
    std::vector<core::EntityId> loaded;
    registry_.each<PendingHaulerLinks>(
        [&](core::EntityId id, PendingHaulerLinks&) { loaded.push_back(id); });
    const core::EntityId headquarters = find_headquarters(registry_);
    for (const core::EntityId id : loaded) {
        const PendingHaulerLinks links = *registry_.get<PendingHaulerLinks>(id);
        registry_.remove<PendingHaulerLinks>(id);
        const RouteKey route{building_at(registry_, links.from), building_at(registry_, links.to)};
        const Route* found = network_.find(route);
        Worker* worker = registry_.get<Worker>(id);
        if (worker == nullptr) {
            continue;
        }
        if (found == nullptr || found->kind != RouteKind::Logistic || headquarters == 0) {
            worker->state = WorkerState::Idle;
            worker->workplace = 0;
            continue;
        }
        worker->state = WorkerState::Working;
        registry_.add(id, Hauler{route, links.mode, links.joined});
        if (links.joined) {
            RouteFollower loop{route, RouteEnd::Loop};
            loop.forward = links.forward;
            follow(registry_, id, loop);
            continue;
        }
        follow(registry_, id, RouteFollower{RouteKey{headquarters, route.from}});
    }
}

void HaulSystem::update(float) {
    resolve_loaded();
    std::vector<core::EntityId> haulers;
    registry_.each<Hauler>([&](core::EntityId id, Hauler&) { haulers.push_back(id); });
    for (const core::EntityId id : haulers) {
        Hauler& hauler = *registry_.get<Hauler>(id);
        const Route* route = network_.find(hauler.route);
        if (route == nullptr || !registry_.has<Building>(hauler.route.from) ||
            !registry_.has<Building>(hauler.route.to)) {
            send_home(id);
            continue;
        }
        const Position* position = registry_.get<Position>(id);
        Inventory* carried = registry_.get<Inventory>(id);
        if (!route->reachable() || position == nullptr || carried == nullptr) {
            continue;
        }
        const TileCoord tile = tile_containing(position->point);
        if (!hauler.joined) {
            if (tile == route->tiles.front()) {
                hauler.joined = true;
                follow(registry_, id, RouteFollower{hauler.route, RouteEnd::Loop});
            }
            continue;
        }
        const bool at_storage = tile == route->tiles.front();
        const bool at_producer = tile == route->tiles.back();
        if (!at_storage && !at_producer) {
            continue;
        }
        Inventory* here =
            registry_.get<Inventory>(at_storage ? hauler.route.from : hauler.route.to);
        if (here == nullptr) {
            continue;
        }
        const bool at_source = (hauler.mode == HaulMode::Take) == at_producer;
        if (at_source) {
            load(*carried, *here);
        } else {
            unload(*carried, *here);
        }
    }
}

} // namespace world
