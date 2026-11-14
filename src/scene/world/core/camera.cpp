#include "scene/world/core/camera.hpp"

namespace world {

IsoProjection Camera::projection(float screen_width, float screen_height) const {
    // Project with the origin at the screen's corner, then shift so the centre lands in the middle.
    IsoProjection projection;
    const ScreenPoint center_offset = projection.world_to_screen(center);
    projection.origin = {screen_width / 2.0f - center_offset.x,
                         screen_height / 2.0f - center_offset.y};
    return projection;
}

} // namespace world
