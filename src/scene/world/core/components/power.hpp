#pragma once

#include <optional>

#include "scene/world/core/items/item.hpp"

namespace world {

// The kinds of power generators.
enum class GeneratorType { Solar, Coal };

// How a building generates electrical power: up to `power_kw`. With an `input` it needs
// `rate` stacks of it per second from its inventory for full power, and its output follows
// the share of that rate it actually gets (its efficiency); with none (solar) it always
// gives `power_kw`.
struct GeneratorSpec {
    GeneratorType type = GeneratorType::Solar;
    std::optional<ItemStack> input = std::nullopt;
    float rate = 0.0f; // input stacks per second at full power
    float power_kw = 0.0f;
};

// Seconds over which a fuelled generator's efficiency follows its fuel supply, so fuel
// delivered one item at a time gives a steady output instead of bursts.
inline constexpr float FuelSmoothingTime = 10.0f;

// A building that generates electrical power (see GeneratorSpec). Set by the PowerSystem,
// none of it saved: `fuel` = input stacks burnt but not used yet, `efficiency` (0..1) =
// smoothed share of the needed fuel rate it got, `output_kw` = power_kw * efficiency,
// `running` = output_kw > 0.
struct Generator {
    GeneratorSpec spec;
    float fuel = 0.0f;
    float efficiency = 0.0f;
    float output_kw = 0.0f;
    bool running = true;
};

// A building that needs `demand_kw` of power to work. The PowerSystem sets `powered`:
// false while the network cannot supply every consumer. It only says whether power
// arrives; whether the building works is its BuildingStatus.
struct PowerConsumer {
    float demand_kw = 0.0f;
    bool powered = true;
};

} // namespace world
