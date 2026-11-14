#pragma once

#include "core/registry.hpp"
#include "scene/world/renderer.hpp"

namespace scene::world {

// Draws every entity that has a Position as a cube with its label (plus the state for
// characters) through the isometric projection, with world (0, 0) at the centre of the
// screen. The registry must outlive the renderer.
class EntityRenderer : public Renderer {
public:
    explicit EntityRenderer(const core::Registry& registry) : registry_(registry) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const core::Registry& registry_;
};

} // namespace scene::world
