#include "graphics/sprite_sheet.hpp"

namespace graphics {

std::optional<Sprite> SpriteSheet::frame(int index) const {
    if (index < 0 || index >= count_ || columns_ <= 0) {
        return std::nullopt;
    }
    const auto width = static_cast<float>(frame_width_);
    const auto height = static_cast<float>(frame_height_);
    const ui::Rect source{static_cast<float>(index % columns_) * width,
                          static_cast<float>(index / columns_) * height, width, height};
    return Sprite{texture_, source};
}

std::vector<Sprite> SpriteSheet::frames(int first, int count) const {
    std::vector<Sprite> result;
    for (int index = first; index < first + count; ++index) {
        if (auto sprite = frame(index)) {
            result.push_back(std::move(*sprite));
        }
    }
    return result;
}

} // namespace graphics
