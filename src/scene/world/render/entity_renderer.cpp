#include "scene/world/render/entity_renderer.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "ui/theme.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/isometric.hpp"

namespace scene::world {

namespace {
constexpr float MarkerSize = 24.0f;
constexpr float CubeTopHeight = 10.0f;
constexpr float LabelGap = 4.0f;
constexpr int LabelFontSize = 16;
constexpr ui::Color CubeFrontColor{200, 160, 50, 255};
constexpr ui::Color CubeTopColor{240, 210, 110, 255};
} // namespace

void EntityRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    ::world::IsoProjection projection;
    projection.origin = {screen.width / 2.0f, screen.height / 2.0f};

    // Back to front: entities further down the screen are drawn later.
    struct Marker {
        ::world::WorldPoint point;
        std::string label;
    };
    std::vector<Marker> markers;
    for (const core::Entity& entity : registry_.entities()) {
        const auto* position = registry_.get<::world::Position>(entity.id);
        if (position == nullptr) {
            continue;
        }
        // Characters show their state next to the label: "Player (Moving)".
        std::string label = entity.label;
        if (const auto* character = registry_.get<::world::Character>(entity.id)) {
            label += std::string(" (") + ::world::to_string(character->state) + ")";
        }
        markers.push_back({position->point, std::move(label)});
    }
    std::sort(markers.begin(), markers.end(), [](const Marker& a, const Marker& b) {
        return ::world::depth_of(a.point) < ::world::depth_of(b.point);
    });

    for (const Marker& marker : markers) {
        const ::world::ScreenPoint center = projection.world_to_screen(marker.point);
        // A cube seen from the front: a lighter top face over the front face, which is
        // centred on the entity's position.
        const ui::Rect front{center.x - MarkerSize / 2.0f, center.y - MarkerSize / 2.0f,
                             MarkerSize, MarkerSize};
        const ui::Rect top{front.x, front.y - CubeTopHeight, MarkerSize, CubeTopHeight};
        canvas.fill_rect(top, CubeTopColor);
        canvas.fill_rect(front, CubeFrontColor);
        const ui::Size text = canvas.measure_text(marker.label, LabelFontSize);
        canvas.draw_text(marker.label,
                         {center.x - text.width / 2.0f, top.y - LabelGap - text.height},
                         LabelFontSize, ui::theme::Muted);
    }
}

} // namespace scene::world
