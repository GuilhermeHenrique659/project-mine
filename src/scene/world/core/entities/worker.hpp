#pragma once

#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// Archetype name of a worker in the entity factory and in saves.
inline constexpr const char* WorkerArchetype = "worker";

// A worker is a character (see spawn_character) with the Worker marker, idle.
core::EntityId spawn_worker(core::Registry& registry, std::string label, WorldPoint position);

} // namespace world
