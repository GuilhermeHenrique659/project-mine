#pragma once

#include <optional>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// How many rings of tiles around a recruiter's footprint are searched for a free spawn tile.
inline constexpr int MaxSpawnRing = 8;

// Recruits soldiers: on SoldierRecruitRequested at a finished (no ConstructionSite) and
// powered Recruiter, it spawns one soldier on the centre of the nearest free tile around
// the building (not a building, not water, no unit on it), ring by ring up to
// MaxSpawnRing; on the building itself when none is free. Everything passed must outlive it.
class RecruitSystem : public System {
public:
    RecruitSystem(core::Registry& registry, event::EventBus& bus, const EntityFactory& factory,
                  const Terrain& terrain);

    void update(float) override {}

private:
    void recruit(core::EntityId building);
    std::optional<TileCoord> spawn_tile(const Building& building) const;

    core::Registry& registry_;
    const EntityFactory& factory_;
    const Terrain& terrain_;
    event::ScopedSubscription subscription_;
};

} // namespace world
