#include "scene/world/core/construction/construction.hpp"

namespace world {

std::string_view building_type_key(BuildingType type) {
    switch (type) {
        case BuildingType::Mine: return "Mine";
        case BuildingType::Headquarters: break;
    }
    return "Headquarters";
}

std::optional<BuildingType> parse_building_type(std::string_view key) {
    if (key == building_type_key(BuildingType::Headquarters)) {
        return BuildingType::Headquarters;
    }
    if (key == building_type_key(BuildingType::Mine)) {
        return BuildingType::Mine;
    }
    return std::nullopt;
}

const std::vector<ConstructionGroup>& construction_catalog() {
    static const std::vector<ConstructionGroup> catalog{
        {ConstructionCategory::Special, "Special", "icon_tab_special",
         {{"Headquarters", "icon_headquarters", BuildingType::Headquarters, {}, 0.0f, true, false},
          {"Storage", "icon_storage"}}},
        {ConstructionCategory::Mine, "Mines", "icon_tab_mines",
         {{"Mine", "icon_mine", BuildingType::Mine, {{items::IronOre, 100}}, 300.0f, false, true,
           true, 10, Recipe{std::nullopt, std::nullopt, 0.5f}}}},
        {ConstructionCategory::Factory, "Factories", "icon_tab_factories",
         {{"Smelter", "icon_smelter"}, {"Assembler", "icon_assembler"}}},
        {ConstructionCategory::Military, "Military", "icon_tab_military",
         {{"Barracks", "icon_barracks"}, {"Turret", "icon_turret"}, {"Wall", "icon_wall"}}},
    };
    return catalog;
}

const Construction* find_construction(BuildingType type) {
    for (const auto& group : construction_catalog()) {
        for (const auto& construction : group.constructions) {
            if (construction.type == type) {
                return &construction;
            }
        }
    }
    return nullptr;
}

} // namespace world
