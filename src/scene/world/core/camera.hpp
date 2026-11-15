#pragma once

#include "scene/world/core/isometric.hpp"

namespace world {

// The point of the world the view is centred on. Raylib-free: it knows nothing of the
// window, only of the screen size handed to projection().
struct Camera {
    // Zoom limits: how small and how big a tile may be drawn, relative to its normal size.
    static constexpr float MinZoom = 0.5f;
    static constexpr float MaxZoom = 2.0f;

    WorldPoint center{};
    // Scale of the tiles, 1 = normal size; always within [MinZoom, MaxZoom].
    float zoom = 1.0f;

    // Zooms in for positive `steps` (mouse wheel notches) and out for negative ones.
    void zoom_by(float steps);

    // The projection for a screen of the given size: `center` lands on the middle of it.
    IsoProjection projection(float screen_width, float screen_height) const;
};

} // namespace world
