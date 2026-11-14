#include "scene/scene.hpp"

namespace scene {

Scene::Scene(std::unique_ptr<Model> model, std::unique_ptr<View> view,
             std::unique_ptr<Controller> controller)
    : model_(std::move(model)), view_(std::move(view)), controller_(std::move(controller)) {}

void Scene::update(float delta_time) {
    if (controller_) {
        controller_->handle_input();
    }
    if (model_) {
        model_->update(delta_time);
    }
    if (view_) {
        view_->update();
    }
}

void Scene::draw() const {
    if (!view_) {
        return;
    }
    view_->draw();
}

} // namespace scene
