#include "scene/world/render/building_details.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/power.hpp"
#include "text/parse.hpp"

namespace scene::world {

namespace {

std::string state_text(const ::world::BuildingStatus& status) {
    if (status.state == ::world::BuildingState::Working) {
        return "State: Working";
    }
    switch (status.reason) {
        case ::world::IdleReason::NoPower: return "State: Idle (no power)";
        case ::world::IdleReason::NoResource: return "State: Idle (no ore)";
        case ::world::IdleReason::NoWorkers: return "State: Idle (no workers)";
        case ::world::IdleReason::MissingInput: return "State: Idle (missing input)";
        case ::world::IdleReason::OutputFull: return "State: Idle (output full)";
        case ::world::IdleReason::None: break;
    }
    return "State: Idle";
}

} // namespace

std::vector<std::string> building_details(const core::Registry& registry, core::EntityId id) {
    std::vector<std::string> lines;
    if (const auto* status = registry.get<::world::BuildingStatus>(id)) {
        lines.push_back(state_text(*status));
    }
    if (const auto* consumer = registry.get<::world::PowerConsumer>(id)) {
        lines.push_back("Power: " + text::format(consumer->demand_kw) + " kW");
    }
    if (const auto* generator = registry.get<::world::Generator>(id)) {
        lines.push_back("Generates: " + text::format(generator->spec.power_kw) + " kW");
    }
    return lines;
}

} // namespace scene::world
