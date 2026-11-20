#pragma once

#include "core/registry.hpp"
#include "scene/world/core/system.hpp"

namespace world {

// Balances the single power network every frame. Finished generators (Generator, no
// ConstructionSite) supply their power_kw; one with an input burns input.count items of
// it from its SolidInventory every 1 / rate seconds and supplies nothing while it lacks them.
// Finished consumers (PowerConsumer, no ConstructionSite) are all unpowered when their
// total demand exceeds the supply, and powered again once it is covered. The registry must
// outlive it. Runs before the ProductionSystem.
class PowerSystem : public System {
public:
    explicit PowerSystem(core::Registry& registry);

    void update(float delta_time) override;

    // Power supplied and demanded during the last update, in kW.
    float supply_kw() const { return supply_kw_; }
    float demand_kw() const { return demand_kw_; }

private:
    core::Registry& registry_;
    float supply_kw_ = 0.0f;
    float demand_kw_ = 0.0f;
};

} // namespace world
