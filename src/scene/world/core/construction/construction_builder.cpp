#include "scene/world/core/construction/construction_builder.hpp"

#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/worker.hpp"

namespace world {

ConstructionBuilder::ConstructionBuilder(std::string name, std::string icon) {
    construction_.name = std::move(name);
    construction_.icon = std::move(icon);
}

ConstructionBuilder& ConstructionBuilder::type(BuildingType type) {
    construction_.type = type;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::size(int width, int height) {
    construction_.size = TileSize{width, height};
    return *this;
}

ConstructionBuilder& ConstructionBuilder::cost(Item item, int count) {
    construction_.cost.push_back({item, count});
    return *this;
}

ConstructionBuilder& ConstructionBuilder::effort(float work) {
    construction_.effort = work;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::needs_headquarters() {
    construction_.rules.push_back(std::make_shared<NeedsHeadquartersRule>());
    return *this;
}

ConstructionBuilder& ConstructionBuilder::needs_ore() {
    construction_.rules.push_back(std::make_shared<NeedsOreRule>());
    return *this;
}

ConstructionBuilder& ConstructionBuilder::unique() {
    construction_.rules.push_back(std::make_shared<UniqueRule>());
    return *this;
}

ConstructionBuilder& ConstructionBuilder::consumes(Item item, int count, std::size_t slots) {
    recipe().inputs.push_back({item, count});
    sections_.emplace_back(slots, item.type);
    return *this;
}

ConstructionBuilder& ConstructionBuilder::produces(Item item, int count) {
    recipe().output = item;
    recipe().output_count = count;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::mines_ore() {
    recipe().output = std::nullopt;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::crafts_per_second(float rate) {
    recipe().rate = rate;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::storage(std::size_t general_slots) {
    general_slots_ = general_slots;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::workers(int needed) {
    workers_ = needed;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::power(float demand_kw) {
    power_kw_ = demand_kw;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::houses(int workers) {
    houses_ = workers;
    return *this;
}

ConstructionBuilder& ConstructionBuilder::solar(float power_kw) {
    generator_ = GeneratorSpec{GeneratorType::Solar, std::nullopt, 0.0f, power_kw};
    return *this;
}

ConstructionBuilder& ConstructionBuilder::burns(Item fuel, float per_second, float power_kw,
                                                std::size_t slots) {
    generator_ = GeneratorSpec{GeneratorType::Coal, ItemStack{fuel, 1}, per_second, power_kw};
    sections_.emplace_back(slots, fuel.type);
    return *this;
}

Recipe& ConstructionBuilder::recipe() {
    if (!recipe_) {
        recipe_ = Recipe{};
    }
    return *recipe_;
}

Construction ConstructionBuilder::build() const {
    Construction construction = construction_;
    Blueprint& blueprint = construction.blueprint;
    if (general_slots_ || !sections_.empty()) {
        Inventory inventory{general_slots_.value_or(0)};
        for (const auto& [slots, item] : sections_) {
            inventory.append(slots, item);
        }
        blueprint.with(inventory);
    }
    if (recipe_) {
        blueprint.with(Producer{*recipe_});
    }
    if (generator_) {
        blueprint.with(Generator{*generator_});
    }
    if (power_kw_) {
        blueprint.with(PowerConsumer{*power_kw_});
    }
    if (houses_) {
        blueprint.with(Housing{*houses_});
    }
    if (recipe_) {
        blueprint.with(BuildingStatus{});
    }
    if (workers_) {
        blueprint.with(Staffing{*workers_});
    }
    return construction;
}

} // namespace world
