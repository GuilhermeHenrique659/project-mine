#include "scene/world/core/construction/build_rules.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

namespace {

// Whether any tile the construction would cover on `origin` is water or taken.
bool footprint_blocked(const core::Registry& registry, const Construction& construction,
                       TileCoord origin, const Terrain& terrain) {
    for (const TileCoord tile : footprint_tiles(Building{{}, origin, construction.size})) {
        if (terrain.tile_at(tile) == TileType::Water || building_at(registry, tile) != 0) {
            return true;
        }
    }
    return false;
}

bool can_pay(const std::vector<ItemStack>& cost, const SolidInventory* payer) {
    for (const ItemStack& stack : cost) {
        if (payer == nullptr || payer->count(stack.item) < stack.count) {
            return false;
        }
    }
    return true;
}

} // namespace

BuildResult check_unlocked(const core::Registry& registry, const Construction& construction) {
    if (!construction.type) {
        return BuildResult::NotBuildable;
    }
    for (const auto& rule : construction.rules) {
        const BuildResult result = rule->check_unlocked(registry, *construction.type);
        if (result != BuildResult::Ok) {
            return result;
        }
    }
    return BuildResult::Ok;
}

BuildResult can_build(const core::Registry& registry, const Construction& construction,
                      TileCoord tile, const Terrain& terrain, const SolidInventory* payer) {
    const BuildResult unlocked = check_unlocked(registry, construction);
    if (unlocked != BuildResult::Ok) {
        return unlocked;
    }
    if (footprint_blocked(registry, construction, tile, terrain)) {
        return BuildResult::TileBlocked;
    }
    for (const auto& rule : construction.rules) {
        const BuildResult result = rule->check_site(terrain, tile);
        if (result != BuildResult::Ok) {
            return result;
        }
    }
    if (!can_pay(construction.cost, payer)) {
        return BuildResult::NotEnoughResources;
    }
    return BuildResult::Ok;
}

BuildResult build(core::Registry& registry, const EntityFactory& factory,
                  const Construction& construction, TileCoord tile, const Terrain& terrain,
                  core::EntityId payer) {
    SolidInventory* inventory = registry.get<SolidInventory>(payer);
    const BuildResult result = can_build(registry, construction, tile, terrain, inventory);
    if (result != BuildResult::Ok) {
        return result;
    }
    for (const ItemStack& stack : construction.cost) {
        inventory->remove(stack.item, stack.count);
    }
    const auto id = factory.spawn(registry, BuildingArchetype, construction.name);
    make_building(registry, *id, construction, tile);
    return BuildResult::Ok;
}

} // namespace world
