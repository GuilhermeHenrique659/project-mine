#include "scene/world/world_view.hpp"

#include <raylib.h>

namespace scene::world {

WorldView::WorldView(std::vector<std::unique_ptr<Renderer>> renderers)
    : renderers_(std::move(renderers)) {}

void WorldView::update() {
    for (auto it = renderers_.rbegin(); it != renderers_.rend(); ++it) {
        if ((*it)->visible()) {
            (*it)->update();
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
    const ui::Size screen{static_cast<float>(GetScreenWidth()),
                          static_cast<float>(GetScreenHeight())};
    draw(canvas, screen);
}

} // namespace scene::world
