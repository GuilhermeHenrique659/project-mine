#include "scene/world/render/shot_renderer.hpp"

#include <cmath>
#include <cstdint>

namespace scene::world {

namespace {
constexpr ui::Color TraceColor{255, 220, 90, 255};
// Width of the line on screen at the default zoom, in pixels.
constexpr float TraceWidth = 2.0f;
} // namespace

ShotRenderer::ShotRenderer(const WorldModel& model) : model_(model) {}

void ShotRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    const ::world::IsoProjection projection = model_.camera().projection(screen.width, screen.height);
    for (const ShotTrace& trace : model_.shot_traces()) {
        const ::world::ScreenPoint a = projection.world_to_screen(trace.from);
        const ::world::ScreenPoint b = projection.world_to_screen(trace.to);
        const float length = std::hypot(b.x - a.x, b.y - a.y);
        if (length <= 0.0f) {
            continue;
        }
        // Half the width, perpendicular to the line.
        const float nx = -(b.y - a.y) / length * TraceWidth * 0.5f;
        const float ny = (b.x - a.x) / length * TraceWidth * 0.5f;
        const float fade = 1.0f - trace.age / ShotTraceDuration;
        ui::Color color = TraceColor;
        color.a = static_cast<std::uint8_t>(255.0f * std::fmax(0.0f, fade));
        const ui::Point p1{a.x + nx, a.y + ny};
        const ui::Point p2{a.x - nx, a.y - ny};
        const ui::Point p3{b.x - nx, b.y - ny};
        const ui::Point p4{b.x + nx, b.y + ny};
        canvas.fill_triangle(p1, p2, p3, color);
        canvas.fill_triangle(p1, p3, p4, color);
    }
}

} // namespace scene::world
