#pragma once

#include "core/registry.hpp"
#include "input/input_handler.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Steers every PlayerControlled entity with a Mover from the held move actions
// (MoveUp/Down/Left/Right). The actions are in screen space (up = towards the top of
// the screen) and are turned into a world direction; no keys held means standing still.
// It must run before the systems that move entities. The registry and the input must
// outlive the system.
class PlayerInputSystem : public System {
public:
    PlayerInputSystem(core::Registry& registry, const input::InputHandler& input)
        : registry_(registry), input_(input) {}

    void update(float delta_time) override;

private:
    core::Registry& registry_;
    const input::InputHandler& input_;
};

} // namespace world
