#pragma once

#include <cstddef>
#include <string>

#include "core/registry.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// Archetype name of a worker in the entity factory and in saves.
inline constexpr const char* WorkerArchetype = "worker";

// Slots of a worker's inventory: what a hauler carries in one trip.
inline constexpr std::size_t WorkerInventorySlots = 1;

// A worker is a unit (see spawn_unit) with the Worker marker, idle, and an inventory of
// WorkerInventorySlots. What work it does is its worker_type().
core::EntityId spawn_worker(core::Registry& registry, std::string label, WorldPoint position);

} // namespace world
