#pragma once

#include <vector>

#include "core/registry.hpp"
#include "ui/composite.hpp"
#include "ui/label.hpp"

namespace scene::world {

// The Energy page of the player menu, read from ::world::power_report() on every layout:
// the summary ("Produced: N kW", "Consumed: N kW", plus a warning while overloaded),
// then "Generators (n)" with one "<name> - N kW" line each (" (stopped)" when not
// running), then "Consumers (n)" the same way (" (no power)" when unpowered); "None"
// under an empty list. Text only, no interaction. A column of labels, rebuilt only when the
// lines' layout changes; otherwise their texts and colours are updated. The registry must
// outlive it.
class EnergyPage : public ui::Composite {
public:
    explicit EnergyPage(const core::Registry& registry);

private:
    void sync(const ui::Canvas& canvas) const override;

    // Where a line sits: its indent and the space above it (what the tree is built from).
    struct Slot {
        float indent = 0.0f;
        float gap_before = 0.0f;
        bool operator==(const Slot&) const = default;
    };

    const core::Registry& registry_;
    mutable std::vector<Slot> slots_;
    mutable std::vector<ui::Label*> labels_; // owned by the tree, one per line
};

} // namespace scene::world
