#include "scene/world/core/logistics_report.hpp"

#include <algorithm>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/construction/construction.hpp"

namespace world {

namespace {

LogisticEnd end_of(const core::Registry& registry, core::EntityId id) {
    const Building* building = registry.get<Building>(id);
    if (building == nullptr) {
        return {"?", {}};
    }
    const Construction* construction = find_construction(building->type);
    return {construction != nullptr ? construction->name : "?", building->tile};
}

} // namespace

std::vector<LogisticEntry> logistics_report(const core::Registry& registry,
                                            const RouteNetwork& network) {
    std::vector<LogisticEntry> entries;
    for (const Route& route : network.routes()) {
        if (route.kind != RouteKind::Logistic) {
            continue;
        }
        LogisticEntry entry{route.key, end_of(registry, route.key.from),
                            end_of(registry, route.key.to), route.tiles.size()};
        if (const Building* to = registry.get<Building>(route.key.to)) {
            entry.to_type = to->type;
        }
        // The registry is const here, so walk the entities instead of each<Hauler>().
        for (const core::Entity& entity : registry.entities()) {
            const Hauler* hauler = registry.get<Hauler>(entity.id);
            if (hauler == nullptr || !(hauler->route == route.key)) {
                continue;
            }
            ++entry.haulers;
            if (!entry.mode) {
                entry.mode = hauler->mode;
            }
        }
        entries.push_back(entry);
    }
    std::stable_sort(entries.begin(), entries.end(),
                     [](const LogisticEntry& a, const LogisticEntry& b) {
                         return a.to_type < b.to_type;
                     });
    return entries;
}

} // namespace world
