#pragma once

#include "core/registry.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Keeps the camera centred on the PlayerControlled entity that has a Position. It must
// run after the systems that move entities. The registry and the camera must outlive it.
class CameraFollowSystem : public System {
public:
    CameraFollowSystem(const core::Registry& registry, Camera& camera)
        : registry_(registry), camera_(camera) {}

    void update(float delta_time) override;

private:
    const core::Registry& registry_;
    Camera& camera_;
};

} // namespace world
