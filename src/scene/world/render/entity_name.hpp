#pragma once

#include <string>

#include "core/registry.hpp"
#include "text/parse.hpp"

namespace scene::world {

// How panels name a building: its name plus its entity id, "Coal Generator #12".
inline std::string entity_name(const std::string& name, core::EntityId id) {
    return name + " #" + text::format(id);
}

} // namespace scene::world
