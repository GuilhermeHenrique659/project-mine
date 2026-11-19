#pragma once

#include <memory>
#include <vector>

#include "input/pointer_gate.hpp"
#include "scene/scene.hpp"
#include "scene/world/render/renderer.hpp"
#include "ui/ui.hpp"

namespace scene::world {

// The in-game screen: a list of renderers (the entities, the pause menu, ...) given by
// whoever composes the scene. They are drawn in list order, first = bottom, and updated
// topmost first; a renderer that is not visible() is skipped by both.
class WorldView : public scene::View {
public:
    // `gate` is the pointer the renderers read; the view resets it every frame and claims it
    // for the topmost renderer that captures it. Without one, every renderer sees every click.
    explicit WorldView(std::vector<std::unique_ptr<Renderer>> renderers,
                       std::shared_ptr<input::PointerGate> gate = nullptr);

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
    std::shared_ptr<input::PointerGate> gate_;
};

} // namespace scene::world
