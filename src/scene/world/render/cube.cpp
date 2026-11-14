#include "scene/world/render/cube.hpp"

namespace scene::world {

namespace {
// Height of the top face relative to the front face's side.
constexpr float TopRatio = 0.4f;
} // namespace

ui::Rect Cube::bounds(::world::ScreenPoint center) const {
    const float top_height = size_ * TopRatio;
    return {center.x - size_ / 2.0f, center.y - size_ / 2.0f - top_height, size_,
            size_ + top_height};
}

void Cube::draw(ui::Canvas& canvas, ::world::ScreenPoint center) const {
    const float top_height = size_ * TopRatio;
    const ui::Rect body = bounds(center);
    canvas.fill_rect({body.x, body.y, size_, top_height}, top_);
    canvas.fill_rect({body.x, body.y + top_height, size_, size_}, front_);
}

} // namespace scene::world
