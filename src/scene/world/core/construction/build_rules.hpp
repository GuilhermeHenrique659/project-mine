#pragma once

#include "core/registry.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/construction/build_rule.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/isometric.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace world {

// The rules that do not depend on where or who: NotBuildable, then the check_unlocked()
// of each of the construction's rules in order, else Ok. The build bar uses it to disable entries.
BuildResult check_unlocked(const core::Registry& registry, const Construction& construction);

// Every rule, for placing `construction` on `tile` paid from `payer` (null = pays nothing,
// so only an empty cost passes), in order: check_unlocked(), TileBlocked, the
// check_site() of each of its rules, NotEnoughResources.
BuildResult can_build(const core::Registry& registry, const Construction& construction,
                      TileCoord tile, const Terrain& terrain, const Inventory* payer);

// Applies every rule of can_build(), with the cost paid from the Inventory of `payer`;
// when they pass, takes the cost and spawns the building (through the factory, so it is
// saved), labelled with the construction's name.
BuildResult build(core::Registry& registry, const EntityFactory& factory,
                  const Construction& construction, TileCoord tile, const Terrain& terrain,
                  core::EntityId payer);

} // namespace world
