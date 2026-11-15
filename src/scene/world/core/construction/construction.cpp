#include "scene/world/core/construction/construction.hpp"

namespace world {

const std::vector<ConstructionGroup>& construction_catalog() {
    static const std::vector<ConstructionGroup> catalog{
        {ConstructionCategory::Special, "Special", "icon_tab_special",
         {{"Headquarters", "icon_headquarters"}, {"Storage", "icon_storage"}}},
        {ConstructionCategory::Mine, "Mines", "icon_tab_mines",
         {{"Iron Mine", "icon_iron_mine"},
          {"Copper Mine", "icon_copper_mine"},
          {"Coal Mine", "icon_coal_mine"}}},
        {ConstructionCategory::Factory, "Factories", "icon_tab_factories",
         {{"Smelter", "icon_smelter"}, {"Assembler", "icon_assembler"}}},
        {ConstructionCategory::Military, "Military", "icon_tab_military",
         {{"Barracks", "icon_barracks"}, {"Turret", "icon_turret"}, {"Wall", "icon_wall"}}},
    };
    return catalog;
}

} // namespace world
