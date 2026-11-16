#pragma once

namespace game {

// Longest frame the game loop passes on: after a hitch (a breakpoint, a window drag) the
// simulation catches up at most this much instead of jumping.
inline constexpr float MaxFrameTime = 0.25f;

// The frame time limited to [0, MaxFrameTime].
float clamp_frame_time(float seconds);

// Turns variable frame times into a whole number of fixed ticks, carrying the remainder
// to the next frame, so the simulation gives the same result at any frame rate.
class FixedStep {
public:
    explicit FixedStep(float step_seconds);

    // Adds a frame's time; returns how many ticks of step() to run now.
    int advance(float frame_seconds);

    float step() const { return step_; }

private:
    float step_;
    double accumulated_ = 0.0;
};

} // namespace game
