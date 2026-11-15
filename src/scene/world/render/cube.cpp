#include "scene/world/render/cube.hpp"

#include <cstdint>

namespace scene::world {

namespace {
// Height of the vertical edges relative to the width: about a true cube in a 2:1 projection.
constexpr float HeightRatio = 0.6f;
// How much darker the right face is than the left one.
constexpr float RightShade = 0.7f;

ui::Color shade(ui::Color color, float factor) {
    const auto scale = [factor](std::uint8_t channel) {
        return static_cast<std::uint8_t>(static_cast<float>(channel) * factor);
    };
    return {scale(color.r), scale(color.g), scale(color.b), color.a};
}

ui::Point to_point(::world::ScreenPoint p) { return {p.x, p.y}; }

void fill_quad(ui::Canvas& canvas, const std::array<::world::ScreenPoint, 4>& corners,
               ui::Color color) {
    canvas.fill_triangle(to_point(corners[0]), to_point(corners[1]), to_point(corners[2]), color);
    canvas.fill_triangle(to_point(corners[0]), to_point(corners[2]), to_point(corners[3]), color);
}
} // namespace

float Cube::height() const { return size_ * HeightRatio; }

CubeFaces Cube::faces(::world::ScreenPoint ground) const {
    const float half_width = size_ / 2.0f;
    const float half_height = size_ / 4.0f;
    const float rise = height();
    // The base diamond, centred on the ground point.
    const ::world::ScreenPoint left{ground.x - half_width, ground.y};
    const ::world::ScreenPoint front{ground.x, ground.y + half_height};
    const ::world::ScreenPoint right{ground.x + half_width, ground.y};
    const auto up = [rise](::world::ScreenPoint p) { return ::world::ScreenPoint{p.x, p.y - rise}; };
    const ::world::ScreenPoint back_top{ground.x, ground.y - half_height - rise};
    return {
        {back_top, up(right), up(front), up(left)},
        {up(left), up(front), front, left},
        {up(right), up(front), front, right},
    };
}

ui::Rect Cube::bounds(::world::ScreenPoint ground) const {
    const float top = ground.y - size_ / 4.0f - height();
    return {ground.x - size_ / 2.0f, top, size_, size_ / 2.0f + height()};
}

void Cube::draw(ui::Canvas& canvas, ::world::ScreenPoint ground) const {
    const CubeFaces corners = faces(ground);
    fill_quad(canvas, corners.top, top_);
    fill_quad(canvas, corners.left, front_);
    fill_quad(canvas, corners.right, shade(front_, RightShade));
}

} // namespace scene::world
