#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "core/registry.hpp"
#include "graphics/game_sprites.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/render/entity_styles.hpp"
#include "scene/world/render/renderer.hpp"

namespace scene::world {

// Draws every entity that has a Position, through the isometric projection with the
// camera's centre at the middle of the screen, the way its EntityStyle says: its sprite, or
// its cube when it has none, and its label above. Entities whose body does not reach the
// screen are skipped before any label is built. The registry, the camera, the sprites and
// the styles must outlive the renderer.
class EntityRenderer : public Renderer {
public:
    // Seconds of animation played so far; none = always the first frame.
    using AnimationClock = std::function<float()>;

    EntityRenderer(const core::Registry& registry, const ::world::Camera& camera,
                   const graphics::GameSprites& sprites, const EntityStyles& styles,
                   AnimationClock clock = {})
        : registry_(registry),
          camera_(camera),
          sprites_(sprites),
          styles_(styles),
          clock_(std::move(clock)) {}
    // The game's styles.
    EntityRenderer(const core::Registry& registry, const ::world::Camera& camera,
                   const graphics::GameSprites& sprites, AnimationClock clock = {})
        : EntityRenderer(registry, camera, sprites, EntityStyles::standard(), std::move(clock)) {}
    // The game's styles, cubes only.
    EntityRenderer(const core::Registry& registry, const ::world::Camera& camera,
                   AnimationClock clock = {})
        : EntityRenderer(registry, camera, graphics::GameSprites::none(), std::move(clock)) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    // One entity that is on screen this frame.
    struct Marker {
        const core::Entity* entity; // in the registry, valid during draw()
        const EntityStyle* style;
        ::world::ScreenPoint center;
        EntityLook look;
        ui::Rect body; // where the sprite or the cube is drawn
    };

    const core::Registry& registry_;
    const ::world::Camera& camera_;
    const graphics::GameSprites& sprites_;
    const EntityStyles& styles_;
    AnimationClock clock_;
    // Reused every frame so drawing does not allocate once they have grown.
    mutable std::vector<Marker> markers_;
    mutable std::vector<::world::DepthBox> boxes_;
};

} // namespace scene::world
