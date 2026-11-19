#pragma once

#include <memory>
#include <utility>

#include "input/pointer.hpp"

namespace input {

// A pointer shared by stacked layers (the world screen's renderers). The topmost layer
// under the pointer claim()s it; from then on, for the rest of the frame, the layers below
// see no click, no held button and no wheel, only the position (for hover). reset() starts
// a new frame.
class PointerGate : public Pointer {
public:
    explicit PointerGate(std::shared_ptr<const Pointer> source) : source_(std::move(source)) {}

    PointerPosition position() const override { return source_->position(); }
    bool released() const override { return !claimed_ && source_->released(); }
    bool down() const override { return !claimed_ && source_->down(); }
    bool secondary_down() const override { return !claimed_ && source_->secondary_down(); }
    bool secondary_released() const override {
        return !claimed_ && source_->secondary_released();
    }
    float scroll() const override { return claimed_ ? 0.0f : source_->scroll(); }

    void claim() { claimed_ = true; }
    void reset() { claimed_ = false; }

private:
    std::shared_ptr<const Pointer> source_;
    bool claimed_ = false;
};

} // namespace input
