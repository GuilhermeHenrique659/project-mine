#pragma once

#include <utility>
#include <vector>

#include "graphics/sprite.hpp"

namespace graphics {

// A sequence of sprites played at a fixed rate. It keeps no clock: the caller holds the
// elapsed time (add the frame delta each update) and asks for the sprite at that time.
class Animation {
public:
    Animation(std::vector<Sprite> frames, float frames_per_second, bool loop)
        : frames_(std::move(frames)), frames_per_second_(frames_per_second), loop_(loop) {}

    // The sprite to show after `elapsed_seconds`. Looping animations start over after the
    // last frame; the others stay on it. Null when there are no frames.
    const Sprite* sprite_at(float elapsed_seconds) const;

private:
    std::vector<Sprite> frames_;
    float frames_per_second_;
    bool loop_;
};

} // namespace graphics
