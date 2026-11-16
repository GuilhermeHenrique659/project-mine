#include "game/fixed_step.hpp"

#include <algorithm>

namespace game {

namespace {
// Share of a tick accepted as rounding error, so 0.5 s at 1/60 s gives 30 ticks, not 29.
constexpr double TickTolerance = 1e-4;
} // namespace

float clamp_frame_time(float seconds) { return std::clamp(seconds, 0.0f, MaxFrameTime); }

FixedStep::FixedStep(float step_seconds) : step_(step_seconds) {}

int FixedStep::advance(float frame_seconds) {
    if (step_ <= 0.0f || frame_seconds <= 0.0f) {
        return 0;
    }
    accumulated_ += frame_seconds;
    const int ticks = static_cast<int>(accumulated_ / step_ + TickTolerance);
    accumulated_ = std::max(0.0, accumulated_ - ticks * static_cast<double>(step_));
    return ticks;
}

} // namespace game
