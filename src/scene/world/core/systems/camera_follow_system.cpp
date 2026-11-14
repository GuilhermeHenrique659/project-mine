#include "scene/world/core/systems/camera_follow_system.hpp"

#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"

namespace world {

void CameraFollowSystem::update(float /*delta_time*/) {
    for (const core::Entity& entity : registry_.entities()) {
        if (!registry_.has<PlayerControlled>(entity.id)) {
            continue;
        }
        const auto* position = registry_.get<Position>(entity.id);
        if (position == nullptr) {
            continue;
        }
        camera_.center = position->point;
        return;
    }
}

} // namespace world
