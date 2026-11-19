#pragma once

#include "core/registry.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Steers every PathFollower: points its Mover at the centre of the tile it walks to and
// moves on once it is within a step of it. On the last tile it stands exactly on the
// centre, stops and loses the PathFollower. Runs before UnitTrafficSystem and
// CharacterMoveSystem. The registry must outlive it.
class PathFollowSystem : public System {
public:
    explicit PathFollowSystem(core::Registry& registry) : registry_(registry) {}

    void update(float delta_time) override;

private:
    core::Registry& registry_;
};

} // namespace world
