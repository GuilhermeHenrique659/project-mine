#include "scene/world/core/camera.hpp"

#include <algorithm>
#include <cmath>

namespace world {

void Camera::zoom_by(float steps) {
    // Each notch scales the zoom by the same factor, so in and out feel symmetric.
    constexpr float ZoomPerStep = 1.1f;
    zoom = std::clamp(zoom * std::pow(ZoomPerStep, steps), MinZoom, MaxZoom);
}

IsoProjection Camera::projection(float screen_width, float screen_height) const {
    // Project with the origin at the screen's corner, then shift so the centre lands in the middle.
    IsoProjection projection;
    projection.tile_width *= zoom;
    projection.tile_height *= zoom;
    const ScreenPoint center_offset = projection.world_to_screen(center);
    projection.origin = {screen_width / 2.0f - center_offset.x,
                         screen_height / 2.0f - center_offset.y};
    return projection;
}

} // namespace world
