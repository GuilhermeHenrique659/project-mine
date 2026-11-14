#pragma once

namespace world {

// A piece of game logic that runs once per frame over the entities of a registry.
// A system receives what it depends on (registry, event bus) in its constructor, so
// update() only needs the time step; the owner keeps all systems in one list and calls
// update() on each in order.
class System {
public:
    virtual ~System() = default;

    virtual void update(float delta_time) = 0;
};

} // namespace world
