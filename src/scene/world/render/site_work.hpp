#pragma once

#include <memory>

#include "input/pointer.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"

namespace scene::world {

// Lets the player build by hand: while the left button is held over a construction site,
// the player works on it (WorldModel::work_on(), which only progresses within reach);
// otherwise the player stops. Hidden (and not working) while paused or while the player
// menu is open. It draws nothing; draw() only records the screen size used to pick the
// tile under the pointer. The model must outlive it.
class SiteWork : public Renderer {
public:
    SiteWork(WorldModel& model, std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    void update() override;
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    mutable ui::Size screen_;
};

} // namespace scene::world
