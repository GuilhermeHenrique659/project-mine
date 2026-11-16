#include "scene/world/core/construction/build_rules.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/entities/building.hpp"

namespace world {

namespace {

bool any_building(const core::Registry& registry, auto&& matches) {
    for (const core::Entity& entity : registry.entities()) {
        const Building* building = registry.get<Building>(entity.id);
        if (building != nullptr && matches(*building)) {
            return true;
        }
    }
    return false;
}

bool can_pay(const std::vector<ItemStack>& cost, const Inventory* payer) {
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
                      TileCoord tile, const Terrain& terrain, const Inventory* payer) {
    const BuildResult unlocked = check_unlocked(registry, construction);
    if (unlocked != BuildResult::Ok) {
        return unlocked;
    }
    if (terrain.tile_at(tile) == TileType::Water ||
        any_building(registry, [tile](const Building& building) { return building.tile == tile; })) {
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
    Inventory* inventory = registry.get<Inventory>(payer);
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
