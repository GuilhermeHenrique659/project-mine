#pragma once

#include "scene/world/core/terrain/terrain.hpp"
#include "scene/world/render/renderer.hpp"
#include "ui/geometry.hpp"

namespace scene::world {

inline constexpr ui::Color GrassColor{76, 160, 62, 255};
inline constexpr ui::Color WaterColor{52, 120, 205, 255};

// Draws the ground: every tile on screen as an isometric diamond, green for grass and
// blue for water, with world (0, 0) at the centre of the screen. Drawn below the
// entities. The terrain must outlive the renderer.
class TerrainRenderer : public Renderer {
public:
    explicit TerrainRenderer(const ::world::Terrain& terrain) : terrain_(terrain) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const ::world::Terrain& terrain_;
};

} // namespace scene::world
