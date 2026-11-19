#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "core/registry.hpp"

namespace world {

// Maps an archetype name to the function that spawns it. Filled at start-up and injected
// (not a singleton, so tests build their own).
class EntityFactory {
public:
    using Spawner = std::function<core::EntityId(core::Registry&, const std::string& label)>;

    // Replaces a spawner already registered under the same name.
    void register_archetype(std::string archetype, Spawner spawner);

    // Creates the entity with the archetype's default components and Persistent{archetype};
    // empty when the archetype is unknown.
    std::optional<core::EntityId> spawn(core::Registry& registry, const std::string& archetype,
                                        const std::string& label) const;

private:
    std::unordered_map<std::string, Spawner> spawners_;
};

// Every archetype of the game: "character" -> spawn_character at (0, 0) and "building" ->
// spawn_building (placed afterwards with make_building()), "worker" and "soldier" at (0, 0).
EntityFactory make_entity_factory();

} // namespace world
