#pragma once

#include "scene/world/core/isometric.hpp"

namespace world {

// The point of the world the view is centred on. Raylib-free: it knows nothing of the
// window, only of the screen size handed to projection().
struct Camera {
    WorldPoint center{};

    // The projection for a screen of the given size: `center` lands on the middle of it.
    IsoProjection projection(float screen_width, float screen_height) const;
};

} // namespace world
