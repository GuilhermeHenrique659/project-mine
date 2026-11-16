#pragma once

#include <optional>

#include "scene/world/core/items/item.hpp"

namespace world {

// The kinds of power generators.
enum class GeneratorType { Solar };

// How a building generates electrical power: `power_kw` while running. With an `input`
// it burns that stack from its inventory every 1 / `rate` seconds and stops without it;
// with none (solar) it always runs.
struct GeneratorSpec {
    GeneratorType type = GeneratorType::Solar;
    std::optional<ItemStack> input = std::nullopt;
    float rate = 0.0f; // input stacks per second
    float power_kw = 0.0f;
};

// A building that generates electrical power (see GeneratorSpec). `progress` counts
// towards the next consumption of its input (1 = one input stack burnt); it is not saved.
struct Generator {
    GeneratorSpec spec;
    float progress = 0.0f;
};

// A building that needs `demand_kw` of power to work. The PowerSystem sets `powered`:
// false while the network cannot supply every consumer. It only says whether power
// arrives; whether the building works is its BuildingStatus.
struct PowerConsumer {
    float demand_kw = 0.0f;
    bool powered = true;
};

} // namespace world
