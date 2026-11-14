#pragma once

#include <memory>
#include <vector>

#include "scene/scene.hpp"
#include "scene/world/render/renderer.hpp"
#include "ui/ui.hpp"

namespace scene::world {

// The in-game screen: a list of renderers (the entities, the pause menu, ...) given by
// whoever composes the scene. They are drawn in list order, first = bottom, and updated
// topmost first; a renderer that is not visible() is skipped by both.
class WorldView : public scene::View {
public:
    explicit WorldView(std::vector<std::unique_ptr<Renderer>> renderers);

    // Lets the visible renderers poll their input.
    void update() override;

    // Draws the visible renderers on a screen of the given size.
    void draw(ui::Canvas& canvas, ui::Size screen) const;

    // Draws on the raylib window.
    void draw() const override;

private:
    WorldView(const WorldView&) = delete;
    WorldView& operator=(const WorldView&) = delete;

    std::vector<std::unique_ptr<Renderer>> renderers_;
};

} // namespace scene::world
