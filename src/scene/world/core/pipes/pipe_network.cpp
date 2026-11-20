#include "scene/world/core/pipes/pipe_network.hpp"

#include <algorithm>
#include <map>
#include <utility>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/liquid_inventory.hpp"
#include "scene/world/core/occupancy.hpp"

namespace world {

namespace {

using TileKey = std::pair<int, int>;

TileKey key_of(TileCoord tile) { return {tile.x, tile.y}; }

constexpr TileCoord Neighbours[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

bool is_finished(const core::Registry& registry, core::EntityId id) {
    return !registry.has<ConstructionSite>(id);
}

} // namespace

void PipeNetwork::rebuild(const core::Registry& registry) {
    groups_.clear();

    // Finished pipes by tile, and the tiles of every finished tank.
    std::map<TileKey, core::EntityId> pipes;
    std::map<TileKey, core::EntityId> tanks;
    for (const core::Entity& entity : registry.entities()) {
        const Building* building = registry.get<Building>(entity.id);
        if (building == nullptr || !is_finished(registry, entity.id)) {
            continue;
        }
        if (building->type == BuildingType::Pipe) {
            pipes[key_of(building->tile)] = entity.id;
            continue;
        }
        if (!registry.has<LiquidInventory>(entity.id)) {
            continue;
        }
        for (const TileCoord tile : footprint_tiles(*building)) {
            tanks[key_of(tile)] = entity.id;
        }
    }

    // Flood fill the pipes in creation order, so groups come out in a stable order.
    std::map<TileKey, bool> seen;
    for (const core::Entity& entity : registry.entities()) {
        const Building* building = registry.get<Building>(entity.id);
        if (building == nullptr || building->type != BuildingType::Pipe ||
            !is_finished(registry, entity.id) || seen[key_of(building->tile)]) {
            continue;
        }
        PipeGroup group;
        std::vector<TileCoord> pending{building->tile};
        seen[key_of(building->tile)] = true;
        while (!pending.empty()) {
            const TileCoord tile = pending.back();
            pending.pop_back();
            group.pipes.push_back(pipes.at(key_of(tile)));
            for (const TileCoord offset : Neighbours) {
                const TileCoord next{tile.x + offset.x, tile.y + offset.y};
                if (const auto tank = tanks.find(key_of(next)); tank != tanks.end()) {
                    if (std::find(group.members.begin(), group.members.end(), tank->second) ==
                        group.members.end()) {
                        group.members.push_back(tank->second);
                    }
                }
                if (pipes.contains(key_of(next)) && !seen[key_of(next)]) {
                    seen[key_of(next)] = true;
                    pending.push_back(next);
                }
            }
        }
        groups_.push_back(std::move(group));
    }
}

} // namespace world
