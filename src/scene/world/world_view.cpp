#include "scene/world/world_view.hpp"

#include "ui/raylib_canvas.hpp"

namespace scene::world {

WorldView::WorldView(std::vector<std::unique_ptr<Renderer>> renderers,
                     std::shared_ptr<input::PointerGate> gate)
    : renderers_(std::move(renderers)), gate_(std::move(gate)) {}

void WorldView::update() {
    if (gate_) {
        gate_->reset();
    }
    for (auto it = renderers_.rbegin(); it != renderers_.rend(); ++it) {
        Renderer& renderer = **it;
        if (!renderer.visible()) {
            continue;
        }
        // Decided before the update, from the last layout: a click that closes a panel
        // still must not fall through to the world below it.
        const bool captures = gate_ && renderer.captures_pointer(
                                           {gate_->position().x, gate_->position().y});
        renderer.update();
        if (captures) {
            gate_->claim();
        }
    }
}

void WorldView::draw(ui::Canvas& canvas, ui::Size screen) const {
    for (const auto& renderer : renderers_) {
        if (renderer->visible()) {
            renderer->draw(canvas, screen);
        }
    }
}

void WorldView::draw() const {
    ui::RaylibCanvas canvas;
    draw(canvas, ui::RaylibCanvas::screen_size());
}

} // namespace scene::world
