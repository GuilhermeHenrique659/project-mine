#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "graphics/sprite.hpp"

namespace graphics {

// A texture cut into equal frames laid out in a grid, numbered left to right, top to
// bottom starting at 0.
class SpriteSheet {
public:
    SpriteSheet(std::string texture, int frame_width, int frame_height, int columns, int count)
        : texture_(std::move(texture)),
          frame_width_(frame_width),
          frame_height_(frame_height),
          columns_(columns),
          count_(count) {}

    int count() const { return count_; }

    // The frame, or nothing when `index` is outside the sheet.
    std::optional<Sprite> frame(int index) const;

    // Up to `count` consecutive frames from `first` (only the ones that exist), e.g. a
    // walk cycle.
    std::vector<Sprite> frames(int first, int count) const;

private:
    std::string texture_;
    int frame_width_;
    int frame_height_;
    int columns_;
    int count_;
};

} // namespace graphics
