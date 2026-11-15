#include "scene/world/render/entity_renderer.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "ui/theme.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/render/cube.hpp"

namespace scene::world {

namespace {
constexpr float CubeSize = 24.0f;
constexpr float LabelGap = 4.0f;
constexpr int LabelFontSize = 16;
} // namespace

void EntityRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    const ::world::IsoProjection projection = camera_.projection(screen.width, screen.height);

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
        // The cube scales with the zoom, like the tiles.
        const Cube cube(CubeSize * projection.tile_width / ::world::IsoProjection{}.tile_width);
        cube.draw(canvas, center);
        const ui::Rect body = cube.bounds(center);
        const ui::Size text = canvas.measure_text(marker.label, LabelFontSize);
        canvas.draw_text(marker.label,
                         {center.x - text.width / 2.0f, body.y - LabelGap - text.height},
                         LabelFontSize, ui::theme::Muted);
    }
}

} // namespace scene::world
