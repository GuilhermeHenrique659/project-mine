#pragma once

#include "core/registry.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/render/renderer.hpp"

namespace scene::world {

// Name under which the game registers the character texture (see Game::start_up()).
inline constexpr const char* CharacterTexture = "character";

// Draws every entity that has a Position as a cube with its label (plus the state for
// characters) through the isometric projection, with the camera's centre at the middle
// of the screen. A texture registered as CharacterTexture replaces the cube. The registry
// and the camera must outlive the renderer.
class EntityRenderer : public Renderer {
public:
    EntityRenderer(const core::Registry& registry, const ::world::Camera& camera)
        : registry_(registry), camera_(camera) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const core::Registry& registry_;
    const ::world::Camera& camera_;
};

} // namespace scene::world
