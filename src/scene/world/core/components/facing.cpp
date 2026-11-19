#include "scene/world/core/components/facing.hpp"

#include <cmath>

namespace world {

Direction facing_of(WorldPoint direction) {
    if (direction.x == 0.0f && direction.y == 0.0f) {
        return Direction::South;
    }
    // On screen (2:1 tiles) world x goes (+2, +1) and world y (-2, +1), y pointing down.
    const float screen_x = 2.0f * (direction.x - direction.y);
    const float screen_y = direction.x + direction.y;
    // Clockwise angle from screen up, in eighths of a turn.
    constexpr float Pi = 3.14159265358979f;
    const float eighths = std::atan2(screen_x, -screen_y) / (Pi / 4.0f);
    const int index = static_cast<int>(std::lround(eighths));
    return static_cast<Direction>((index % DirectionCount + DirectionCount) % DirectionCount);
}

} // namespace world
