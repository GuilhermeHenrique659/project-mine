#pragma once

#include <map>
#include <string>

#include "core/registry.hpp"

namespace world {

using Fields = std::map<std::string, std::string>;

// Writes and reads the saved data of one kind of component. Each codec owns its keys
// and ignores the others. Entity ids are not saved (restored entities get fresh ids), so
// references between entities would need saved ids later.
class ComponentCodec {
public:
    virtual ~ComponentCodec() = default;

    // Adds the entity's data to `fields` (nothing when it lacks the component).
    virtual void write(const core::Registry& registry, core::EntityId id, Fields& fields) const = 0;

    // Applies saved data on top of the entity's defaults; missing or bad values keep them.
    virtual void read(core::Registry& registry, core::EntityId id, const Fields& fields) const = 0;
};

} // namespace world
