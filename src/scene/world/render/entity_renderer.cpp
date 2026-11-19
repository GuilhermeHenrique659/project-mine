#include "scene/world/render/entity_renderer.hpp"

#include <string>
#include <vector>

#include "ui/theme.hpp"
#include "scene/world/core/components/selected.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/worker_type.hpp"
#include "scene/world/core/worker_report.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/render/cube.hpp"

namespace scene::world {

namespace {
// 24 px at the default zoom: the character's body, as wide as its collision.
constexpr float CubeSize = ::world::CharacterSize * BuildingCubeSize;
const ui::Color BuildingFront{70, 110, 170, 255};
const ui::Color BuildingTop{120, 160, 220, 255};
const ui::Color SiteFront{120, 120, 120, 255};
const ui::Color SiteTop{170, 170, 170, 255};
constexpr float LabelGap = 4.0f;
constexpr int LabelFontSize = 16;
constexpr float SelectionOutline = 2.0f;
} // namespace

void EntityRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    const ::world::IsoProjection projection = camera_.projection(screen.width, screen.height);

    struct Marker {
        ::world::WorldPoint point;
        std::string label;
        bool building = false;
        bool site = false;
        ::world::TileSize footprint;
        bool selected = false;
    };
    std::vector<Marker> markers;
    for (const core::Entity& entity : registry_.entities()) {
        const auto* position = registry_.get<::world::Position>(entity.id);
        // A worker doing its job is inside the building: not drawn.
        if (position == nullptr || ::world::is_at_work(registry_, entity.id)) {
            continue;
        }
        // Characters show their state next to the label: "Player (Moving)".
        std::string label = entity.label;
        // Workers show the work they do instead: "Worker (Miner)", "Worker (Idle)".
        if (registry_.has<::world::Worker>(entity.id)) {
            label += std::string(" (") +
                     ::world::to_string(::world::worker_type(registry_, entity.id)) + ")";
        } else if (const auto* character = registry_.get<::world::Character>(entity.id)) {
            label += std::string(" (") + ::world::to_string(character->state) + ")";
        }
        // Unfinished buildings show how far along they are: "Mine (25%)".
        const auto* site = registry_.get<::world::ConstructionSite>(entity.id);
        if (site != nullptr && site->effort > 0.0f) {
            const int percent = static_cast<int>(100.0f * site->progress / site->effort);
            label += " (" + std::to_string(percent) + "%)";
        }
        const auto* building = registry_.get<::world::Building>(entity.id);
        markers.push_back({position->point, std::move(label), building != nullptr,
                           site != nullptr, building != nullptr ? building->size : ::world::TileSize{},
                           registry_.has<::world::Selected>(entity.id)});
    }
    // Painter's algorithm on what each entity covers: a building its footprint, anything
    // else its point, so a character beside a big building is drawn on the right side of it.
    std::vector<::world::DepthBox> boxes;
    boxes.reserve(markers.size());
    for (const Marker& marker : markers) {
        const float half_x = marker.building ? marker.footprint.width / 2.0f : 0.0f;
        const float half_y = marker.building ? marker.footprint.height / 2.0f : 0.0f;
        boxes.push_back({{marker.point.x - half_x, marker.point.y - half_y},
                         {marker.point.x + half_x, marker.point.y + half_y}});
    }

    for (const std::size_t index : ::world::painter_order(boxes)) {
        const Marker& marker = markers[index];
        const ::world::ScreenPoint center = projection.world_to_screen(marker.point);
        // The cube scales with the zoom, like the tiles.
        const float scale = projection.tile_width / ::world::IsoProjection{}.tile_width;
        // A building covers its whole footprint.
        const float tile = BuildingCubeSize * scale;
        const Cube cube = !marker.building ? Cube(CubeSize * scale)
                          : marker.site    ? Cube::footprint(tile, marker.footprint, SiteFront, SiteTop)
                                           : Cube::footprint(tile, marker.footprint, BuildingFront,
                                                             BuildingTop);
        cube.draw(canvas, center);
        const ui::Rect body = cube.bounds(center);
        // Selected units are outlined (box selection, see UnitCommand).
        if (marker.selected) {
            canvas.stroke_rect(body, ui::theme::Accent, SelectionOutline);
        }
        const ui::Size text = canvas.measure_text(marker.label, LabelFontSize);
        canvas.draw_text(marker.label,
                         {center.x - text.width / 2.0f, body.y - LabelGap - text.height},
                         LabelFontSize, ui::theme::Muted);
    }
}

} // namespace scene::world
