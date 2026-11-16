#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Moves every character (an entity with Character, Position and Mover) along its
// direction at its speed and keeps its state: Moving while it has a direction, Idle
// otherwise. Publishes CharacterStateChanged only when the state changes. Entities
// without the Character component are left alone.
// The direction is normalized so diagonals are not faster.
// With a terrain, the player (PlayerControlled) collides: it cannot move onto water or a
// building's footprint, keeping PlayerRadius from them, and slides along the free axis
// when a diagonal step is blocked. A player already overlapping one moves freely so it can
// get out. Other characters (workers) walk into buildings freely.
// The registry, the bus and the terrain must outlive the system.
class CharacterMoveSystem : public System {
public:
    CharacterMoveSystem(core::Registry& registry, event::EventBus& bus,
                        const Terrain* terrain = nullptr)
        : registry_(registry), bus_(bus), terrain_(terrain) {}

    void update(float delta_time) override;

private:
    // Moves `position` by `step`, unless that puts the player into water or a building.
    void move(core::EntityId id, Position& position, WorldPoint step) const;
    // The furthest point from `from` towards `from + step` the player's body fits at.
    WorldPoint furthest(WorldPoint from, WorldPoint step) const;

    // Precision of the contact search: 1 / 2^12 of a step.
    static constexpr int ContactSearchSteps = 12;

    core::Registry& registry_;
    event::EventBus& bus_;
    const Terrain* terrain_;
};

// How far (in tiles) the player's body reaches from its position: half the drawn
// character (CharacterSize), so it stops where its cube touches.
constexpr float PlayerRadius = CharacterSize / 2.0f;

} // namespace world
