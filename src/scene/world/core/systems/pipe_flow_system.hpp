#pragma once

#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/pipes/pipe_network.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Liquid units each pipe group moves per second, shared by all its members.
inline constexpr float PipeThroughput = 50.0f;

// Moves liquid through the pipes: each group carries up to PipeThroughput units/s from its
// producers (members with a Producer, e.g. an oil pump) to its other members (e.g. a liquid
// storage), taking turns among them so several producers and consumers share the flow.
// What a consumer refuses (full, or holding another liquid) stays in the producer. The
// network is rebuilt on the first update and after any BuildingPlaced or BuildingCompleted.
class PipeFlowSystem : public System {
public:
    PipeFlowSystem(core::Registry& registry, event::EventBus& bus);

    void update(float delta_time) override;

    const PipeNetwork& network() const { return network_; }

private:
    core::Registry& registry_;
    PipeNetwork network_;
    // Per group: the flow not moved yet (under one unit) and whose turn it is.
    std::vector<float> budgets_;
    std::vector<std::size_t> turns_;
    bool dirty_ = true;
    event::ScopedSubscription placed_;
    event::ScopedSubscription completed_;
};

} // namespace world
