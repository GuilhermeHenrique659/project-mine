#include "scene/world/core/construction/construction.hpp"

#include "scene/world/core/construction/construction_builder.hpp"

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/worker.hpp"

namespace world {

std::string_view building_type_key(BuildingType type) {
    switch (type) {
        case BuildingType::Mine: return "Mine";
        case BuildingType::Storage: return "Storage";
        case BuildingType::Steelworks: return "Steelworks";
        case BuildingType::ElectronicFactory: return "ElectronicFactory";
        case BuildingType::House: return "House";
        case BuildingType::CoalGenerator: return "CoalGenerator";
        case BuildingType::ToolFactory: return "ToolFactory";
        case BuildingType::AmmoFactory: return "AmmoFactory";
        case BuildingType::WeaponFactory: return "WeaponFactory";
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
    if (key == building_type_key(BuildingType::Storage)) {
        return BuildingType::Storage;
    }
    if (key == building_type_key(BuildingType::Steelworks)) {
        return BuildingType::Steelworks;
    }
    if (key == building_type_key(BuildingType::ElectronicFactory)) {
        return BuildingType::ElectronicFactory;
    }
    if (key == building_type_key(BuildingType::House)) {
        return BuildingType::House;
    }
    if (key == building_type_key(BuildingType::CoalGenerator)) {
        return BuildingType::CoalGenerator;
    }
    if (key == building_type_key(BuildingType::ToolFactory)) {
        return BuildingType::ToolFactory;
    }
    if (key == building_type_key(BuildingType::AmmoFactory)) {
        return BuildingType::AmmoFactory;
    }
    if (key == building_type_key(BuildingType::WeaponFactory)) {
        return BuildingType::WeaponFactory;
    }
    return std::nullopt;
}

const std::vector<ConstructionGroup>& construction_catalog() {
    static const std::vector<ConstructionGroup> catalog{
        {ConstructionCategory::Special, "Special", "icon_tab_special",
         {
             ConstructionBuilder{"Headquarters", "icon_headquarters"}
                 .type(BuildingType::Headquarters).size(5, 5)
                 .unique()
                 .solar(1000.0f)
                 .build(),
             ConstructionBuilder{"Storage", "icon_storage"}
                 .type(BuildingType::Storage).size(2, 2)
                 .cost(items::IronOre, 200).effort(600.0f).needs_headquarters()
                 .storage(120)
                 .power(20.0f)
                 .build(),
             ConstructionBuilder{"House", "icon_house"}
                 .type(BuildingType::House).size(2, 2)
                 .cost(items::Steel, 100).effort(300.0f).needs_headquarters()
                 .houses(50)
                 .power(20.0f)
                 .build(),
             ConstructionBuilder{"Coal Generator", "icon_coal_generator"}
                 .type(BuildingType::CoalGenerator).size(3, 3)
                 .cost(items::Steel, 200).cost(items::CopperOre, 100).effort(1200.0f)
                 .needs_headquarters()
                 .burns(items::Coal, 1.0f, 1000.0f)
                 .build(),
         }},
        {ConstructionCategory::Mine, "Mines", "icon_tab_mines",
         {
             ConstructionBuilder{"Mine", "icon_mine"}
                 .type(BuildingType::Mine).size(2, 2)
                 .cost(items::IronOre, 100).effort(300.0f).needs_headquarters().needs_ore()
                 .mines_ore().crafts_per_second(4.0f)
                 .storage(10).workers(10).power(100.0f)
                 .build(),
         }},
        {ConstructionCategory::Factory, "Factories", "icon_tab_factories",
         {
             ConstructionBuilder{"Steelworks", "icon_steelworks"}
                 .type(BuildingType::Steelworks).size(3, 3)
                 .cost(items::IronOre, 300).effort(1200.0f).needs_headquarters()
                 .consumes(items::IronOre, 2).consumes(items::Coal, 2)
                 .produces(items::Steel).crafts_per_second(2.0f)
                 .storage(10).workers(50).power(200.0f)
                 .build(),
             ConstructionBuilder{"Electronic Factory", "icon_electronic_factory"}
                 .type(BuildingType::ElectronicFactory).size(3, 3)
                 .cost(items::Steel, 200).effort(1200.0f).needs_headquarters()
                 .consumes(items::CopperOre, 2).consumes(items::IronOre, 2)
                 .produces(items::Electronics).crafts_per_second(2.0f)
                 .storage(10).workers(50).power(200.0f)
                 .build(),
             ConstructionBuilder{"Tool Factory", "icon_tool_factory"}
                 .type(BuildingType::ToolFactory).size(3, 3)
                 .cost(items::Steel, 200).cost(items::Electronics, 100).effort(1200.0f)
                 .needs_headquarters()
                 .consumes(items::Electronics, 1).consumes(items::Steel, 1)
                 .produces(items::Tool).crafts_per_second(2.0f)
                 .storage(10).workers(50).power(300.0f)
                 .build(),
             ConstructionBuilder{"Ammo Factory", "icon_ammo_factory"}
                 .type(BuildingType::AmmoFactory).size(3, 3)
                 .cost(items::Steel, 200).cost(items::Tool, 100).effort(1200.0f)
                 .needs_headquarters()
                 .consumes(items::Coal, 1).consumes(items::IronOre, 1)
                 .consumes(items::CopperOre, 1)
                 .produces(items::Ammo, 2).crafts_per_second(2.0f)
                 .storage(10).workers(50).power(300.0f)
                 .build(),
             ConstructionBuilder{"Weapon Factory", "icon_weapon_factory"}
                 .type(BuildingType::WeaponFactory).size(3, 3)
                 .cost(items::Steel, 200).cost(items::Tool, 100).effort(1200.0f)
                 .needs_headquarters()
                 .consumes(items::Steel, 1).consumes(items::Tool, 1)
                 .produces(items::Weapon).crafts_per_second(2.0f)
                 .storage(10).workers(50).power(300.0f)
                 .build(),
             ConstructionBuilder{"Smelter", "icon_smelter"}.build(),
             ConstructionBuilder{"Assembler", "icon_assembler"}.build(),
         }},
        {ConstructionCategory::Military, "Military", "icon_tab_military",
         {
             ConstructionBuilder{"Barracks", "icon_barracks"}.build(),
             ConstructionBuilder{"Turret", "icon_turret"}.build(),
             ConstructionBuilder{"Wall", "icon_wall"}.build(),
         }},
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
