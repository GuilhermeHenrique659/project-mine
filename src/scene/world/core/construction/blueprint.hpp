#pragma once

#include <any>
#include <functional>
#include <vector>

#include "core/registry.hpp"

namespace world {

// The components a finished building is made of (a prefab). Any component type can be
// added with with(), so a new building feature is a new component in the catalog, not a
// new field here. Copyable, so catalog entries can be copied.
class Blueprint {
public:
    // Adds `component` to the blueprint; returns *this to chain calls.
    template <typename C>
    Blueprint& with(C component) {
        parts_.push_back({component, [component](core::Registry& registry, core::EntityId id) {
                              if (!registry.has<C>(id)) {
                                  registry.add(id, component);
                              }
                          }});
        return *this;
    }

    // The blueprint's component of type C, null when it has none.
    template <typename C>
    const C* find() const {
        for (const Part& part : parts_) {
            if (const C* component = std::any_cast<C>(&part.component)) {
                return component;
            }
        }
        return nullptr;
    }

    // Gives `id` a copy of every component; a component the entity already has is kept
    // (a loaded building keeps the inventory read from its save).
    void apply(core::Registry& registry, core::EntityId id) const;

private:
    struct Part {
        std::any component;
        std::function<void(core::Registry&, core::EntityId)> install;
    };

    std::vector<Part> parts_;
};

} // namespace world
