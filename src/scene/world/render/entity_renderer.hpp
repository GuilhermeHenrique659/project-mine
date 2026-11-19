#pragma once

#include <functional>
#include <utility>

#include "core/registry.hpp"
#include "scene/world/core/camera.hpp"
#include "scene/world/render/renderer.hpp"

namespace scene::world {

// Draws every entity that has a Position as a cube with its label (plus the state for
// characters) through the isometric projection, with the camera's centre at the middle
// of the screen. The registry and
// the camera must outlive the renderer.
class EntityRenderer : public Renderer {
public:
    // Seconds of animation played so far (building sprites); none = always the first frame.
    using AnimationClock = std::function<float()>;

    EntityRenderer(const core::Registry& registry, const ::world::Camera& camera,
                   AnimationClock clock = {})
        : registry_(registry), camera_(camera), clock_(std::move(clock)) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const core::Registry& registry_;
    const ::world::Camera& camera_;
    AnimationClock clock_;
};

} // namespace scene::world
