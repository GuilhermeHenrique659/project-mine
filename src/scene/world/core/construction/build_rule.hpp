#pragma once

#include "core/registry.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/items/item.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

enum class BuildResult {
    Ok,
    NotBuildable,       // a catalog entry without a type
    NeedsHeadquarters,  // everything but the headquarters needs one first
    AlreadyExists,      // a unique building that is already built (or being built)
    TileBlocked,        // water or another building
    NeedsOre,           // a mine off an ore tile
    NotEnoughResources, // the payer's inventory lacks part of the cost
};

// True when a headquarters exists (finished or still a construction site).
bool has_headquarters(const core::Registry& registry);

// One extra condition a construction lists to be placed. A new condition is a new
// subclass, added to the catalog entries that need it. check_unlocked() runs before the
// player picks a tile (the build bar disables the entry on failure); check_site() runs
// for a given tile, after the tile is known to be free. Both default to Ok.
class BuildRule {
public:
    virtual ~BuildRule() = default;

    virtual BuildResult check_unlocked(const core::Registry& registry, BuildingType type) const;
    virtual BuildResult check_site(const Terrain& terrain, TileCoord tile) const;
};

// Only one building of its type may exist (AlreadyExists).
class UniqueRule : public BuildRule {
public:
    BuildResult check_unlocked(const core::Registry& registry, BuildingType type) const override;
};

// A headquarters must exist first (NeedsHeadquarters).
class NeedsHeadquartersRule : public BuildRule {
public:
    BuildResult check_unlocked(const core::Registry& registry, BuildingType type) const override;
};

// Stands only on an ore tile whose item is of `category` (NeedsOre): Solid = iron, copper
// or coal (a mine), Liquid = crude oil (an oil pump).
class NeedsOreRule : public BuildRule {
public:
    explicit NeedsOreRule(ItemCategory category = ItemCategory::Solid) : category_(category) {}

    BuildResult check_site(const Terrain& terrain, TileCoord tile) const override;

private:
    ItemCategory category_;
};

} // namespace world
