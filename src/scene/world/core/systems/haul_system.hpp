#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Drives the haulers (see Hauler) of the logistic routes:
// - on HaulersRequested it turns that many idle workers into haulers of the route (a route
//   that is not Logistic is ignored) and sends them along the headquarters' route to the
//   storage; with a negative count it sends that many of the route's haulers back idle;
// - on HaulModeRequested it switches every hauler of the route to the mode;
// - each update, a hauler that reached the storage joins the logistic route in a loop; at
//   the source end (the production building in Take mode, the storage in Put mode) an
//   empty hauler fills its one slot with the first item there; at the other end it unloads
//   what fits. A hauler whose route is gone turns idle and walks back to the headquarters.
// - a loaded hauler (PendingHaulerLinks) gets its Hauler back on its next update: on the
//   loop if it had joined the route, otherwise walking to the storage.
// Runs after CharacterMoveSystem. Everything passed must outlive it.
class HaulSystem : public System {
public:
    HaulSystem(core::Registry& registry, event::EventBus& bus, const RouteNetwork& network);

    void update(float delta_time) override;

private:
    void assign(RouteKey route, HaulMode mode, int count);
    void release(RouteKey route, int count);
    // Turns the PendingHaulerLinks of loaded workers into Haulers back on their path.
    void resolve_loaded();
    // Takes the hauler off its route: idle, walking back to the headquarters.
    void send_home(core::EntityId worker);

    core::Registry& registry_;
    const RouteNetwork& network_;
    event::ScopedSubscription haulers_subscription_;
    event::ScopedSubscription mode_subscription_;
};

} // namespace world
