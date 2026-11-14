#pragma once

#include "core/registry.hpp"
#include "scene/world/render/renderer.hpp"

namespace scene::world {

// Name under which the game registers the character texture (see Game::start_up()).
inline constexpr const char* CharacterTexture = "character";

// Draws every entity that has a Position as a cube with its label (plus the state for
// characters) through the isometric projection, with world (0, 0) at the centre of the
// screen. A texture registered as CharacterTexture replaces the cube. The registry must
// outlive the renderer.
class EntityRenderer : public Renderer {
public:
    explicit EntityRenderer(const core::Registry& registry) : registry_(registry) {}

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const core::Registry& registry_;
};

} // namespace scene::world
