#include "graphics/animation.hpp"

#include <algorithm>
#include <cmath>

namespace graphics {

const Sprite* Animation::sprite_at(float elapsed_seconds) const {
    if (frames_.empty()) {
        return nullptr;
    }
    if (elapsed_seconds <= 0.0f || frames_per_second_ <= 0.0f) {
        return &frames_.front();
    }
    const auto last = frames_.size() - 1;
    const auto step = static_cast<std::size_t>(std::floor(elapsed_seconds * frames_per_second_));
    return &frames_[loop_ ? step % frames_.size() : std::min(step, last)];
}

} // namespace graphics
