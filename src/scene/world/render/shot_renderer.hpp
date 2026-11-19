#pragma once

#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"

namespace scene::world {

// Draws each recent shot (WorldModel::shot_traces()) as a thin yellow line from the shooter
// to the target that fades out over ShotTraceDuration. The model must outlive it.
class ShotRenderer : public Renderer {
public:
    explicit ShotRenderer(const WorldModel& model);

    void draw(ui::Canvas& canvas, ui::Size screen) const override;

private:
    const WorldModel& model_;
};

} // namespace scene::world
