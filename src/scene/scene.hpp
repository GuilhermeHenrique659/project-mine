#pragma once

#include <memory>

namespace scene {

// Holds the state and rules of a scene. Knows nothing about input or rendering.
class Model {
public:
    virtual ~Model() = default;
    virtual void update(float delta_time) = 0;
};

// Renders a model. Concrete views keep a reference to their concrete model. A view with
// parts that poll input of their own (the mouse) does it in update(), which runs after
// the model's, never inside draw().
class View {
public:
    virtual ~View() = default;
    virtual void update() {}
    virtual void draw() const = 0;
};

// Translates player input into actions on the model.
class Controller {
public:
    virtual ~Controller() = default;
    virtual void handle_input() = 0;
};

// A scene is the composition of a model, a view and a controller.
// The model is declared first so it outlives the view and controller that reference it.
class Scene {
public:
    Scene(std::unique_ptr<Model> model, std::unique_ptr<View> view,
          std::unique_ptr<Controller> controller);

    void update(float delta_time);
    void draw() const;

private:
    std::unique_ptr<Model> model_;
    std::unique_ptr<View> view_;
    std::unique_ptr<Controller> controller_;
};

} // namespace scene
