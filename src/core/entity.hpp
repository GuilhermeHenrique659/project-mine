#pragma once

#include <cstdint>
#include <string>

namespace core {

using EntityId = std::uint32_t;

// What every entity has. Anything else (position, speed, ...) is a component.
struct Entity {
    EntityId id = 0;
    std::string label;
};

} // namespace core
