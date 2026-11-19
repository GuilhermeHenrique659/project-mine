#pragma once

#include <functional>
#include <utility>

#include "scene/world/core/camera.hpp"
#include "scene/world/core/terrain/terrain.hpp"
#include "scene/world/render/renderer.hpp"
#include "ui/geometry.hpp"

namespace scene::world {

inline constexpr ui::Color GrassColor{76, 160, 62, 255};
inline constexpr ui::Color WaterColor{52, 120, 205, 255};
inline constexpr ui::Color IronOreColor{120, 140, 165, 255};
inline constexpr ui::Color CopperOreColor{224, 125, 45, 255};
inline constexpr ui::Color CoalColor{30, 30, 34, 255};

// The colour a tile of this type is drawn with.
ui::Color tile_color(::world::TileType type);

// Draws the ground: every tile on screen as an isometric diamond in its type's colour
// (green grass, blue water, grey / orange / black iron / copper / coal ore), with the
// sprite of tile_sprite() on top when its texture loads; the camera's centre is at the
// middle of the screen. Drawn below the entities. The terrain and the camera must outlive the renderer.
class TerrainRenderer : public Renderer {
public:
    // Seconds of animation played so far (water waves); none = always the first frame.
    using AnimationClock = std::function<float()>;

    TerrainRenderer(const ::world::Terrain& terrain, const ::world::Camera& camera,
                    AnimationClock clock = {})
        : terrain_(terrain), camera_(camera), clock_(std::move(clock)) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const ::world::Terrain& terrain_;
    const ::world::Camera& camera_;
    AnimationClock clock_;
};

} // namespace scene::world
