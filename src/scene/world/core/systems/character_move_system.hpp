#pragma once

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// Moves every character (an entity with Character, Position and Mover) along its
// direction at its speed and keeps its state: Moving while it has a direction, Idle
// otherwise. Publishes CharacterStateChanged only when the state changes. Entities
// without the Character component are left alone.
// The direction is normalized so diagonals are not faster.
// With a terrain, the player (PlayerControlled) and every Unit collide: they cannot move
// onto water or a building's footprint, keeping PlayerRadius from them, and slide along
// the free axis when a diagonal step is blocked. A unit walks into the two buildings its
// RouteFollower's route links (where it works, builds or unloads). A body already
// overlapping an obstacle moves freely so it can get out; so does a ghost unit (one stuck
// too long, see UnitTraffic). Other characters walk freely.
// The registry, the bus and the terrain must outlive the system.
class CharacterMoveSystem : public System {
public:
    CharacterMoveSystem(core::Registry& registry, event::EventBus& bus,
                        const Terrain* terrain = nullptr)
        : registry_(registry), bus_(bus), terrain_(terrain) {}

    void update(float delta_time) override;

private:
    // Whether the entity's body collides (the player and units).
    bool collides(core::EntityId id) const;
    // Moves `position` by `step`, unless that puts a colliding body into an obstacle.
    void move(core::EntityId id, Position& position, WorldPoint step) const;
    // The furthest point from `from` towards `from + step` the body fits at, with the
    // `passable` buildings walkable.
    WorldPoint furthest(WorldPoint from, WorldPoint step, RouteKey passable) const;

    // Precision of the contact search: 1 / 2^12 of a step.
    static constexpr int ContactSearchSteps = 12;

    core::Registry& registry_;
    event::EventBus& bus_;
    const Terrain* terrain_;
};

// How far (in tiles) the player's body reaches from its position: half the drawn
// character (CharacterSize), so it stops where its cube touches.
constexpr float PlayerRadius = CharacterSize / 2.0f;

// A unit's body is the same size as the player's.
constexpr float UnitRadius = PlayerRadius;

} // namespace world
