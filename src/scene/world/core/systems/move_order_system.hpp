#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Carries out UnitsMoveRequested: every soldier of the order gets a PathFollower with the
// A* path (find_path, around water and buildings) from its tile to its own tile of the
// formation around the goal. A soldier with no path there stops where it is. Everything
// passed must outlive it.
class MoveOrderSystem : public System {
public:
    MoveOrderSystem(core::Registry& registry, event::EventBus& bus, const Terrain& terrain);

    // Orders are carried out when published; the walking is the PathFollowSystem's.
    void update(float) override {}

private:
    void order(const UnitsMoveRequested& request);

    core::Registry& registry_;
    const Terrain& terrain_;
    event::ScopedSubscription subscription_;
};

} // namespace world
