#include "scene/world/core/construction/blueprint.hpp"

namespace world {

void Blueprint::apply(core::Registry& registry, core::EntityId id) const {
    for (const Part& part : parts_) {
        part.install(registry, id);
    }
}

} // namespace world
