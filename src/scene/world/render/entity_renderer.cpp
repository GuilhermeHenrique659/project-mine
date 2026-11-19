#include "scene/world/render/entity_renderer.hpp"

#include <string>

#include "graphics/sprite.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/selected.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float LabelGap = 4.0f;
constexpr int LabelFontSize = 16;
constexpr float SelectionOutline = 2.0f;
// How far a label may stick out of its body's sides: an entity this close to the screen
// is kept so its label does not pop in and out at the edge.
constexpr float LabelOverhang = 128.0f;

bool reaches_screen(ui::Rect body, ui::Size screen) {
    const float top = body.y - LabelGap - static_cast<float>(LabelFontSize);
    return body.x + body.width + LabelOverhang >= 0.0f && body.x - LabelOverhang <= screen.width &&
           body.y + body.height >= 0.0f && top <= screen.height;
}

// What the entity covers on the ground, for the painter's algorithm: its footprint, or its
// point, so a character beside a big building is drawn on the right side of it.
::world::DepthBox depth_of(::world::WorldPoint point, const EntityLook& look) {
    const float half_x = look.footprint ? look.footprint->width / 2.0f : 0.0f;
    const float half_y = look.footprint ? look.footprint->height / 2.0f : 0.0f;
    return {{point.x - half_x, point.y - half_y}, {point.x + half_x, point.y + half_y}};
}
} // namespace

void EntityRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    const ::world::IsoProjection projection = camera_.projection(screen.width, screen.height);
    // Sprites and cubes scale with the zoom, like the tiles.
    const LookContext context{sprites_, clock_ ? clock_() : 0.0f,
                              projection.tile_width / ::world::IsoProjection{}.tile_width};

    markers_.clear();
    boxes_.clear();
    for (const core::Entity& entity : registry_.entities()) {
        const auto* position = registry_.get<::world::Position>(entity.id);
        if (position == nullptr) {
            continue;
        }
        const EntityStyle& style = styles_.style_for(registry_, entity.id);
        if (!style.visible(registry_, entity.id)) {
            continue;
        }
        const ::world::ScreenPoint center = projection.world_to_screen(position->point);
        EntityLook look = style.look(registry_, entity.id, context);
        const ui::Rect body = look.sprite ? look.sprite->dest({center.x, center.y}, context.scale)
                                          : look.cube.bounds(center);
        if (!reaches_screen(body, screen)) {
            continue;
        }
        boxes_.push_back(depth_of(position->point, look));
        markers_.push_back({&entity, &style, center, std::move(look), body});
    }

    for (const std::size_t index : ::world::painter_order(boxes_)) {
        const Marker& marker = markers_[index];
        // The sprite, anchored on the ground point; the cube when there is none or its
        // texture does not load.
        ui::Rect body = marker.body;
        if (!marker.look.sprite || !graphics::draw_frame(canvas, *marker.look.sprite, body)) {
            marker.look.cube.draw(canvas, marker.center);
            body = marker.look.cube.bounds(marker.center);
        }
        // Selected units are outlined (box selection, see UnitCommand).
        if (registry_.has<::world::Selected>(marker.entity->id)) {
            canvas.stroke_rect(body, ui::theme::Accent, SelectionOutline);
        }
        const std::string label = marker.style->label(registry_, *marker.entity);
        const ui::Size text = canvas.measure_text(label, LabelFontSize);
        canvas.draw_text(label, {marker.center.x - text.width / 2.0f, body.y - LabelGap - text.height},
                         LabelFontSize, ui::theme::Muted);
    }
}

} // namespace scene::world
