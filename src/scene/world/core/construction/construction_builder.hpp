#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "scene/world/core/components/power.hpp"
#include "scene/world/core/construction/construction.hpp"

namespace world {

// Reads a catalog entry as a list of facts instead of nested initialisers:
//
//   ConstructionBuilder{"Steelworks", "icon_steelworks"}
//       .type(BuildingType::Steelworks).size(3, 3)
//       .cost(items::IronOre, 300).effort(1200.0f).needs_headquarters()
//       .consumes(items::IronOre, 2).consumes(items::Coal, 2).produces(items::Steel)
//       .crafts_per_second(2.0f).storage(10).workers(50).power(200.0f)
//       .build();
//
// build() turns the facts into the Construction and its Blueprint: an Inventory (general
// slots + one filtered section per input or fuel), a Producer + BuildingStatus when it
// produces, Staffing, PowerConsumer, Generator, Housing and Recruiter when given.
class ConstructionBuilder {
public:
    // Filtered slots given to each recipe input and fuel unless said otherwise.
    static constexpr std::size_t InputSlots = 5;

    ConstructionBuilder(std::string name, std::string icon);

    // What it is and where it goes.
    ConstructionBuilder& type(BuildingType type);
    ConstructionBuilder& size(int width, int height);

    // What it takes to build.
    ConstructionBuilder& cost(Item item, int count);
    ConstructionBuilder& effort(float work);
    ConstructionBuilder& needs_headquarters();
    ConstructionBuilder& needs_ore();
    ConstructionBuilder& unique();

    // What it consumes and makes. Each input gets its own filtered section of `slots`.
    ConstructionBuilder& consumes(Item item, int count, std::size_t slots = InputSlots);
    ConstructionBuilder& produces(Item item, int count = 1);
    // Produces the ore under the building (a mine).
    ConstructionBuilder& mines_ore();
    ConstructionBuilder& crafts_per_second(float rate);

    // What it runs on.
    ConstructionBuilder& storage(std::size_t general_slots);
    ConstructionBuilder& workers(int needed);
    ConstructionBuilder& power(float demand_kw);
    ConstructionBuilder& houses(int workers);
    // Soldiers can be recruited at it (Recruiter).
    ConstructionBuilder& recruits();

    // Power it generates: from nothing, or by burning `fuel` per second.
    ConstructionBuilder& solar(float power_kw);
    ConstructionBuilder& burns(Item fuel, float per_second, float power_kw,
                               std::size_t slots = InputSlots);

    Construction build() const;

private:
    Construction construction_;
    std::optional<std::size_t> general_slots_;
    std::vector<std::pair<std::size_t, ItemType>> sections_;
    std::optional<Recipe> recipe_;
    std::optional<int> workers_;
    std::optional<float> power_kw_;
    std::optional<int> houses_;
    bool recruits_ = false;
    std::optional<GeneratorSpec> generator_;

    Recipe& recipe();
};

} // namespace world
