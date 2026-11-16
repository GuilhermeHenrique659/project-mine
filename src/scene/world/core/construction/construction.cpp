#include "scene/world/core/construction/construction.hpp"

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
    return std::nullopt;
}

const std::vector<ConstructionGroup>& construction_catalog() {
    static const std::vector<ConstructionGroup> catalog{
        {ConstructionCategory::Special, "Special", "icon_tab_special",
         {{"Headquarters", "icon_headquarters", BuildingType::Headquarters, {}, 0.0f,
           {std::make_shared<UniqueRule>()},
           Blueprint{}.with(
               Generator{GeneratorSpec{GeneratorType::Solar, std::nullopt, 0.0f, 1000.0f}}),
           TileSize{5, 5}},
          {"Storage", "icon_storage", BuildingType::Storage, {{items::IronOre, 200}}, 600.0f,
           {std::make_shared<NeedsHeadquartersRule>()},
           Blueprint{}.with(Inventory{120}).with(PowerConsumer{20.0f}),
           TileSize{2, 2}}}},
        {ConstructionCategory::Mine, "Mines", "icon_tab_mines",
         {{"Mine", "icon_mine", BuildingType::Mine, {{items::IronOre, 100}}, 300.0f,
           {std::make_shared<NeedsHeadquartersRule>(), std::make_shared<NeedsOreRule>()},
           Blueprint{}
               .with(Inventory{10})
               .with(Producer{Recipe{{}, std::nullopt, 4.0f}})
               .with(PowerConsumer{100.0f})
               .with(BuildingStatus{})
               .with(Staffing{10}),
           TileSize{2, 2}}}},
        {ConstructionCategory::Factory, "Factories", "icon_tab_factories",
         {{"Steelworks", "icon_steelworks", BuildingType::Steelworks, {{items::IronOre, 300}},
           1200.0f, {std::make_shared<NeedsHeadquartersRule>()},
           Blueprint{}
               .with(Inventory{10}.append(5, ItemType::IronOre).append(5, ItemType::Coal))
               .with(Producer{Recipe{{{items::IronOre, 2}, {items::Coal, 2}}, items::Steel, 2.0f}})
               .with(PowerConsumer{200.0f})
               .with(BuildingStatus{})
               .with(Staffing{50}),
           TileSize{3, 3}},
          {"Electronic Factory", "icon_electronic_factory", BuildingType::ElectronicFactory,
           {{items::Steel, 200}}, 1200.0f, {std::make_shared<NeedsHeadquartersRule>()},
           Blueprint{}
               .with(Inventory{10}.append(5, ItemType::CopperOre).append(5, ItemType::IronOre))
               .with(Producer{Recipe{{{items::CopperOre, 2}, {items::IronOre, 2}},
                                     items::Electronics, 2.0f}})
               .with(PowerConsumer{200.0f})
               .with(BuildingStatus{})
               .with(Staffing{50}),
           TileSize{3, 3}},
          {"Smelter", "icon_smelter"},
          {"Assembler", "icon_assembler"}}},
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
