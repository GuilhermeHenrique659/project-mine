#pragma once

#include <functional>
#include <utility>

#include "graphics/game_sprites.hpp"
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
inline constexpr ui::Color CrudeOilColor{45, 30, 55, 255};

// The colour a tile of this type is drawn with when it has no sprite.
ui::Color tile_color(::world::TileType type);

// Draws the ground: every tile on screen with its sprite from GameSprites::tile() over the
// tile's diamond box, or, when it has none or its texture does not load, as an isometric
// diamond in its type's colour (green grass, blue water, grey / orange / black iron /
// copper / coal ore). The camera's centre is at the middle of the screen. Drawn below the
// entities. The terrain, the camera and the sprites must outlive the renderer.
class TerrainRenderer : public Renderer {
public:
    // Seconds of animation played so far (water waves); none = always the first frame.
    using AnimationClock = std::function<float()>;

    TerrainRenderer(const ::world::Terrain& terrain, const ::world::Camera& camera,
                    const graphics::GameSprites& sprites, AnimationClock clock = {})
        : terrain_(terrain), camera_(camera), sprites_(sprites), clock_(std::move(clock)) {}
    // Plain colours only.
    TerrainRenderer(const ::world::Terrain& terrain, const ::world::Camera& camera,
                    AnimationClock clock = {})
        : TerrainRenderer(terrain, camera, graphics::GameSprites::none(), std::move(clock)) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const ::world::Terrain& terrain_;
    const ::world::Camera& camera_;
    const graphics::GameSprites& sprites_;
    AnimationClock clock_;
};

} // namespace scene::world
