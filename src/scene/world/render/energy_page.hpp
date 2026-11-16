#pragma once

#include "core/registry.hpp"
#include "ui/widget.hpp"

namespace scene::world {

// The Energy page of the player menu, read from ::world::power_report() on every draw:
// the summary ("Produced: N kW", "Consumed: N kW", plus a warning while overloaded),
// then "Generators (n)" with one "<name> - N kW" line each (" (stopped)" when not
// running), then "Consumers (n)" the same way (" (no power)" when unpowered); "None"
// under an empty list. Text only, no interaction. The registry must outlive it.
class EnergyPage : public ui::Widget {
public:
    explicit EnergyPage(const core::Registry& registry);

    ui::Size preferred_size(const ui::Canvas& canvas) const override;
    void draw(ui::Canvas& canvas) const override;

private:
    const core::Registry& registry_;
};

} // namespace scene::world
