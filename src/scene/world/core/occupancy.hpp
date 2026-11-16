#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/isometric.hpp"

namespace world {

// Every tile a building covers (its footprint), origin first, row by row.
std::vector<TileCoord> footprint_tiles(const Building& building);

// The world-space centre of a building's footprint (where its Position goes).
WorldPoint footprint_centre(TileCoord origin, TileSize size);

// Which building covers each tile. Lives in the registry's context and is kept up to date
// by the Building add/remove hooks that track_buildings() installs, so it also follows
// buildings restored from a save.
class Occupancy {
public:
    void insert(core::EntityId id, const Building& building);
    void erase(core::EntityId id, const Building& building);

    // The building covering `tile`, 0 when none.
    core::EntityId at(TileCoord tile) const;

private:
    static std::uint64_t key_of(TileCoord tile);

    std::unordered_map<std::uint64_t, core::EntityId> tiles_;
};

// Indexes the registry's buildings (those it has now and every later change). Calling it
// again does nothing.
void track_buildings(core::Registry& registry);

// The building covering `tile`, 0 when none. O(1) on a tracked registry; an untracked one
// (some tests) is scanned.
core::EntityId building_at(const core::Registry& registry, TileCoord tile);

// How far `point` is from entity `id`: from the nearest edge of its footprint when it is a
// Building (0 inside it), otherwise from its Position. Reach checks use it so a big
// building is reached from beside it, not only near its centre. Infinity when `id` has
// neither.
float distance_to_building(const core::Registry& registry, core::EntityId id, WorldPoint point);

} // namespace world
