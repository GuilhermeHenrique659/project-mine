#include "scene/world/render/entity_renderer.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "ui/theme.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/render/cube.hpp"

namespace scene::world {

namespace {
constexpr float CubeSize = 24.0f;
const ui::Color BuildingFront{70, 110, 170, 255};
const ui::Color BuildingTop{120, 160, 220, 255};
const ui::Color SiteFront{120, 120, 120, 255};
const ui::Color SiteTop{170, 170, 170, 255};
constexpr float LabelGap = 4.0f;
constexpr int LabelFontSize = 16;
} // namespace

void EntityRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    const ::world::IsoProjection projection = camera_.projection(screen.width, screen.height);

    // Back to front: entities further down the screen are drawn later.
    struct Marker {
        ::world::WorldPoint point;
        std::string label;
        bool building = false;
        bool site = false;
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
        // Unfinished buildings show how far along they are: "Mine (25%)".
        const auto* site = registry_.get<::world::ConstructionSite>(entity.id);
        if (site != nullptr && site->effort > 0.0f) {
            const int percent = static_cast<int>(100.0f * site->progress / site->effort);
            label += " (" + std::to_string(percent) + "%)";
        }
        markers.push_back({position->point, std::move(label),
                           registry_.has<::world::Building>(entity.id), site != nullptr});
    }
    std::sort(markers.begin(), markers.end(), [](const Marker& a, const Marker& b) {
        return ::world::depth_of(a.point) < ::world::depth_of(b.point);
    });

    for (const Marker& marker : markers) {
        const ::world::ScreenPoint center = projection.world_to_screen(marker.point);
        // The cube scales with the zoom, like the tiles.
        const float scale = projection.tile_width / ::world::IsoProjection{}.tile_width;
        const Cube cube = !marker.building ? Cube(CubeSize * scale)
                          : marker.site    ? Cube(BuildingCubeSize * scale, SiteFront, SiteTop)
                                           : Cube(BuildingCubeSize * scale, BuildingFront, BuildingTop);
        cube.draw(canvas, center);
        const ui::Rect body = cube.bounds(center);
        const ui::Size text = canvas.measure_text(marker.label, LabelFontSize);
        canvas.draw_text(marker.label,
                         {center.x - text.width / 2.0f, body.y - LabelGap - text.height},
                         LabelFontSize, ui::theme::Muted);
    }
}

} // namespace scene::world
