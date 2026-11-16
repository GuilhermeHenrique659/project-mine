#include "scene/world/core/power_report.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/power.hpp"

namespace world {

PowerReport power_report(const core::Registry& registry) {
    PowerReport report;
    for (const core::Entity& entity : registry.entities()) {
        if (registry.has<ConstructionSite>(entity.id)) {
            continue;
        }
        if (const Generator* generator = registry.get<Generator>(entity.id)) {
            report.generators.push_back(
                {entity.id, entity.label, generator->spec.power_kw, generator->running});
            if (generator->running) {
                report.produced_kw += generator->spec.power_kw;
            }
        }
        if (const PowerConsumer* consumer = registry.get<PowerConsumer>(entity.id)) {
            report.consumers.push_back(
                {entity.id, entity.label, consumer->demand_kw, consumer->powered});
            report.demand_kw += consumer->demand_kw;
        }
    }
    return report;
}

} // namespace world
