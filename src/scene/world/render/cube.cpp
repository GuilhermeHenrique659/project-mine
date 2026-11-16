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

float Cube::height() const { return tall_ * HeightRatio; }

CubeFaces Cube::faces(::world::ScreenPoint ground) const {
    // Half of the base's world-x edge runs (+x/4, +x/8) on screen, half its world-y edge
    // (-y/4, +y/8); the corners are the ground point plus or minus each.
    const float xx = along_x_ / 4.0f;
    const float xy = along_x_ / 8.0f;
    const float yx = along_y_ / 4.0f;
    const float yy = along_y_ / 8.0f;
    const float rise = height();
    const ::world::ScreenPoint left{ground.x - xx - yx, ground.y - xy + yy};
    const ::world::ScreenPoint front{ground.x + xx - yx, ground.y + xy + yy};
    const ::world::ScreenPoint right{ground.x + xx + yx, ground.y + xy - yy};
    const auto up = [rise](::world::ScreenPoint p) { return ::world::ScreenPoint{p.x, p.y - rise}; };
    const ::world::ScreenPoint back_top{ground.x - xx + yx, ground.y - xy - yy - rise};
    return {
        {back_top, up(right), up(front), up(left)},
        {up(left), up(front), front, left},
        {up(right), up(front), front, right},
    };
}

ui::Rect Cube::bounds(::world::ScreenPoint ground) const {
    const CubeFaces corners = faces(ground);
    const float left = corners.left[3].x;
    const float top = corners.top[0].y;
    return {left, top, corners.right[3].x - left, corners.left[2].y - top};
}

void Cube::draw(ui::Canvas& canvas, ::world::ScreenPoint ground) const {
    const CubeFaces corners = faces(ground);
    fill_quad(canvas, corners.top, top_);
    fill_quad(canvas, corners.left, front_);
    fill_quad(canvas, corners.right, shade(front_, RightShade));
}

} // namespace scene::world
