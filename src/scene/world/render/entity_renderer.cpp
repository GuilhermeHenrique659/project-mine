#include "scene/world/render/entity_renderer.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "graphics/texture_manager.hpp"
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

// The cube's front face is centred on the position and its top face sits above it.
ui::Rect cube_rect(::world::ScreenPoint center) {
    return {center.x - MarkerSize / 2.0f, center.y - MarkerSize / 2.0f - CubeTopHeight,
            MarkerSize, MarkerSize + CubeTopHeight};
}

ui::Rect texture_rect(const graphics::Texture& texture, ::world::ScreenPoint center) {
    const auto width = static_cast<float>(texture.width);
    const auto height = static_cast<float>(texture.height);
    return {center.x - width / 2.0f, center.y - height / 2.0f, width, height};
}
} // namespace

void EntityRenderer::draw(ui::Canvas& canvas, ui::Size screen) const {
    const ::world::IsoProjection projection = camera_.projection(screen.width, screen.height);

    // Back to front: entities further down the screen are drawn later.
    struct Marker {
        ::world::WorldPoint point;
        std::string label;
        const graphics::Texture* texture = nullptr;
    };
    std::vector<Marker> markers;
    for (const core::Entity& entity : registry_.entities()) {
        const auto* position = registry_.get<::world::Position>(entity.id);
        if (position == nullptr) {
            continue;
        }
        // Characters show their state next to the label: "Player (Moving)".
        std::string label = entity.label;
        const graphics::Texture* texture = nullptr;
        if (const auto* character = registry_.get<::world::Character>(entity.id)) {
            label += std::string(" (") + ::world::to_string(character->state) + ")";
            texture = graphics::TextureManager::instance().get(CharacterTexture);
        }
        markers.push_back({position->point, std::move(label), texture});
    }
    std::sort(markers.begin(), markers.end(), [](const Marker& a, const Marker& b) {
        return ::world::depth_of(a.point) < ::world::depth_of(b.point);
    });

    for (const Marker& marker : markers) {
        const ::world::ScreenPoint center = projection.world_to_screen(marker.point);
        // The texture (at its own size) or a cube seen from the front: a lighter top face
        // over the front face, centred on the entity's position.
        const ui::Rect body = marker.texture ? texture_rect(*marker.texture, center)
                                             : cube_rect(center);
        if (marker.texture) {
            canvas.draw_texture(*marker.texture, body);
        } else {
            canvas.fill_rect({body.x, body.y, body.width, CubeTopHeight}, CubeTopColor);
            canvas.fill_rect({body.x, body.y + CubeTopHeight, body.width, body.height - CubeTopHeight},
                             CubeFrontColor);
        }
        const ui::Size text = canvas.measure_text(marker.label, LabelFontSize);
        canvas.draw_text(marker.label,
                         {center.x - text.width / 2.0f, body.y - LabelGap - text.height},
                         LabelFontSize, ui::theme::Muted);
    }
}

} // namespace scene::world
