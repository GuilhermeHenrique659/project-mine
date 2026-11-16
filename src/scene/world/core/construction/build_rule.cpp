#include "scene/world/core/construction/build_rule.hpp"

#include "scene/world/core/components/building.hpp"

namespace world {

namespace {

bool any_of_type(const core::Registry& registry, BuildingType type) {
    for (const core::Entity& entity : registry.entities()) {
        const Building* building = registry.get<Building>(entity.id);
        if (building != nullptr && building->type == type) {
            return true;
        }
    }
    return false;
}

} // namespace

bool has_headquarters(const core::Registry& registry) {
    return any_of_type(registry, BuildingType::Headquarters);
}

BuildResult BuildRule::check_unlocked(const core::Registry&, BuildingType) const {
    return BuildResult::Ok;
}

BuildResult BuildRule::check_site(const Terrain&, TileCoord) const {
    return BuildResult::Ok;
}

BuildResult UniqueRule::check_unlocked(const core::Registry& registry, BuildingType type) const {
    return any_of_type(registry, type) ? BuildResult::AlreadyExists : BuildResult::Ok;
}

BuildResult NeedsHeadquartersRule::check_unlocked(const core::Registry& registry,
                                                  BuildingType) const {
    return has_headquarters(registry) ? BuildResult::Ok : BuildResult::NeedsHeadquarters;
}

BuildResult NeedsOreRule::check_site(const Terrain& terrain, TileCoord tile) const {
    return is_ore(terrain.tile_at(tile)) ? BuildResult::Ok : BuildResult::NeedsOre;
}

} // namespace world
