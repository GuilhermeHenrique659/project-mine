#include <gtest/gtest.h>

#include <algorithm>
#include <set>

#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/liquid_inventory.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/construction/construction.hpp"

TEST(ConstructionCatalog, HasSpecialMinesFactoriesAndMilitaryInThatOrder) {
    // Given / When
    const auto& catalog = world::construction_catalog();

    // Then
    ASSERT_EQ(catalog.size(), 4u);
    EXPECT_EQ(catalog[0].category, world::ConstructionCategory::Special);
    EXPECT_EQ(catalog[1].category, world::ConstructionCategory::Mine);
    EXPECT_EQ(catalog[2].category, world::ConstructionCategory::Factory);
    EXPECT_EQ(catalog[3].category, world::ConstructionCategory::Military);
}

TEST(ConstructionCatalog, EveryCategoryOffersConstructionsWithTheirOwnIcons) {
    // Given
    const auto& catalog = world::construction_catalog();
    std::set<std::string> icons;
    std::size_t total = 0;

    // When
    for (const auto& group : catalog) {
        EXPECT_FALSE(group.constructions.empty()) << group.name;
        icons.insert(group.icon);
        ++total;
        for (const auto& construction : group.constructions) {
            icons.insert(construction.icon);
            ++total;
        }
    }

    // Then
    EXPECT_EQ(icons.size(), total);
}

TEST(ConstructionCatalog, StorageCosts200IronOreTakes600WorkAndHolds120SlotsOn20Kw) {
    // Given / When
    const world::Construction* storage = world::find_construction(world::BuildingType::Storage);

    // Then
    ASSERT_NE(storage, nullptr);
    EXPECT_EQ(storage->name, "Storage");
    ASSERT_EQ(storage->cost.size(), 1u);
    EXPECT_EQ(storage->cost[0].item, world::items::IronOre);
    EXPECT_EQ(storage->cost[0].count, 200);
    EXPECT_FLOAT_EQ(storage->effort, 600.0f);
    const auto* inventory = storage->blueprint.find<world::SolidInventory>();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->capacity(), 120u);
    const auto* power = storage->blueprint.find<world::PowerConsumer>();
    ASSERT_NE(power, nullptr);
    EXPECT_FLOAT_EQ(power->demand_kw, 20.0f);
}

TEST(BuildingType, StorageRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::Storage);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "Storage");
    EXPECT_EQ(parsed, world::BuildingType::Storage);
}

TEST(ConstructionCatalog, SteelworksCosts300IronOreTakes1200WorkNeeds50WorkersOn200Kw) {
    // Given / When
    const world::Construction* steelworks =
        world::find_construction(world::BuildingType::Steelworks);

    // Then
    ASSERT_NE(steelworks, nullptr);
    EXPECT_EQ(steelworks->name, "Steelworks");
    ASSERT_EQ(steelworks->cost.size(), 1u);
    EXPECT_EQ(steelworks->cost[0].item, world::items::IronOre);
    EXPECT_EQ(steelworks->cost[0].count, 300);
    EXPECT_FLOAT_EQ(steelworks->effort, 1200.0f);
    const auto* staffing = steelworks->blueprint.find<world::Staffing>();
    ASSERT_NE(staffing, nullptr);
    EXPECT_EQ(staffing->needed, 50);
    const auto* power = steelworks->blueprint.find<world::PowerConsumer>();
    ASSERT_NE(power, nullptr);
    EXPECT_FLOAT_EQ(power->demand_kw, 200.0f);
}

TEST(ConstructionCatalog, SteelworksMakesOneSteelFromTwoCoalAndTwoIronOreAtTwoPerSecond) {
    // Given / When
    const auto* producer = world::find_construction(world::BuildingType::Steelworks)
                               ->blueprint.find<world::Producer>();

    // Then
    ASSERT_NE(producer, nullptr);
    const world::Recipe& recipe = producer->recipe;
    EXPECT_EQ(recipe.output, world::items::Steel);
    EXPECT_FLOAT_EQ(recipe.rate, 2.0f);
    ASSERT_EQ(recipe.inputs.size(), 2u);
    EXPECT_EQ(recipe.inputs[0].item, world::items::IronOre);
    EXPECT_EQ(recipe.inputs[0].count, 2);
    EXPECT_EQ(recipe.inputs[1].item, world::items::Coal);
    EXPECT_EQ(recipe.inputs[1].count, 2);
}

TEST(BuildingType, SteelworksRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::Steelworks);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "Steelworks");
    EXPECT_EQ(parsed, world::BuildingType::Steelworks);
}

TEST(ConstructionCatalog, SteelworksHasGeneralSlotsAndOneSectionForIronOreAndOneForCoal) {
    // Given
    world::SolidInventory inventory =
        *world::find_construction(world::BuildingType::Steelworks)->blueprint.find<world::SolidInventory>();

    // When
    const int iron_left = inventory.add(world::items::IronOre, 1000);
    const int coal_left = inventory.add(world::items::Coal, 1000);
    const int steel_left = inventory.add(world::items::Steel, 2000);

    // Then: 5 slots each for iron and coal, 10 general slots for the steel
    EXPECT_EQ(inventory.capacity(), 20u);
    EXPECT_EQ(iron_left, 500);
    EXPECT_EQ(coal_left, 500);
    EXPECT_EQ(steel_left, 1000);
}

TEST(ConstructionCatalog, CoalGeneratorCosts200Steel100CopperOreTakes1200WorkMakes1000KwFromOneCoalPerSecond) {
    // Given / When
    const world::Construction* generator =
        world::find_construction(world::BuildingType::CoalGenerator);

    // Then
    ASSERT_NE(generator, nullptr);
    EXPECT_EQ(generator->name, "Coal Generator");
    ASSERT_EQ(generator->cost.size(), 2u);
    EXPECT_EQ(generator->cost[0].item, world::items::Steel);
    EXPECT_EQ(generator->cost[0].count, 200);
    EXPECT_EQ(generator->cost[1].item, world::items::CopperOre);
    EXPECT_EQ(generator->cost[1].count, 100);
    EXPECT_FLOAT_EQ(generator->effort, 1200.0f);
    const auto* spec = generator->blueprint.find<world::Generator>();
    ASSERT_NE(spec, nullptr);
    EXPECT_EQ(spec->spec.type, world::GeneratorType::Coal);
    ASSERT_TRUE(spec->spec.input.has_value());
    EXPECT_EQ(spec->spec.input->item, world::items::Coal);
    EXPECT_EQ(spec->spec.input->count, 1);
    EXPECT_FLOAT_EQ(spec->spec.rate, 1.0f);
    EXPECT_FLOAT_EQ(spec->spec.power_kw, 1000.0f);
}

TEST(ConstructionCatalog, CoalGeneratorStoresOnlyCoalInFiveSlots) {
    // Given
    world::SolidInventory inventory = *world::find_construction(world::BuildingType::CoalGenerator)
                                      ->blueprint.find<world::SolidInventory>();

    // When
    const int coal_left = inventory.add(world::items::Coal, 1000);
    const int iron_left = inventory.add(world::items::IronOre, 10);

    // Then
    EXPECT_EQ(inventory.capacity(), 5u);
    EXPECT_EQ(coal_left, 500);
    EXPECT_EQ(iron_left, 10);
}

TEST(BuildingType, CoalGeneratorRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::CoalGenerator);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "CoalGenerator");
    EXPECT_EQ(parsed, world::BuildingType::CoalGenerator);
}

TEST(ConstructionCatalog, ElectronicFactoryCosts200SteelTakes1200WorkNeeds50WorkersOn200Kw) {
    // Given / When
    const world::Construction* factory =
        world::find_construction(world::BuildingType::ElectronicFactory);

    // Then
    ASSERT_NE(factory, nullptr);
    EXPECT_EQ(factory->name, "Electronic Factory");
    ASSERT_EQ(factory->cost.size(), 1u);
    EXPECT_EQ(factory->cost[0].item, world::items::Steel);
    EXPECT_EQ(factory->cost[0].count, 200);
    EXPECT_FLOAT_EQ(factory->effort, 1200.0f);
    const auto* staffing = factory->blueprint.find<world::Staffing>();
    ASSERT_NE(staffing, nullptr);
    EXPECT_EQ(staffing->needed, 50);
    const auto* power = factory->blueprint.find<world::PowerConsumer>();
    ASSERT_NE(power, nullptr);
    EXPECT_FLOAT_EQ(power->demand_kw, 200.0f);
}

TEST(ConstructionCatalog, ElectronicFactoryMakesElectronicsFromTwoCopperOreAndTwoIronOreAtTwoPerSecond) {
    // Given / When
    const world::Construction* factory =
        world::find_construction(world::BuildingType::ElectronicFactory);
    ASSERT_NE(factory, nullptr);
    const auto* producer = factory->blueprint.find<world::Producer>();

    // Then
    ASSERT_NE(producer, nullptr);
    const world::Recipe& recipe = producer->recipe;
    EXPECT_EQ(recipe.output, world::items::Electronics);
    EXPECT_FLOAT_EQ(recipe.rate, 2.0f);
    ASSERT_EQ(recipe.inputs.size(), 2u);
    EXPECT_EQ(recipe.inputs[0].item, world::items::CopperOre);
    EXPECT_EQ(recipe.inputs[0].count, 2);
    EXPECT_EQ(recipe.inputs[1].item, world::items::IronOre);
    EXPECT_EQ(recipe.inputs[1].count, 2);
}

TEST(BuildingType, ElectronicFactoryRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::ElectronicFactory);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "ElectronicFactory");
    EXPECT_EQ(parsed, world::BuildingType::ElectronicFactory);
}

TEST(ConstructionCatalog, ElectronicFactoryHasGeneralSlotsAndOneSectionForCopperOreAndOneForIronOre) {
    // Given
    const world::Construction* factory =
        world::find_construction(world::BuildingType::ElectronicFactory);
    ASSERT_NE(factory, nullptr);
    world::SolidInventory inventory = *factory->blueprint.find<world::SolidInventory>();

    // When
    const int copper_left = inventory.add(world::items::CopperOre, 1000);
    const int iron_left = inventory.add(world::items::IronOre, 1000);
    const int electronics_left = inventory.add(world::items::Electronics, 2000);

    // Then: 5 slots each for copper and iron, 10 general slots for the electronics
    EXPECT_EQ(inventory.capacity(), 20u);
    EXPECT_EQ(copper_left, 500);
    EXPECT_EQ(iron_left, 500);
    EXPECT_EQ(electronics_left, 1000);
}

TEST(ConstructionCatalog, HouseCosts100SteelTakes300WorkHouses50WorkersOn20Kw) {
    // Given / When
    const world::Construction* house = world::find_construction(world::BuildingType::House);

    // Then
    ASSERT_NE(house, nullptr);
    EXPECT_EQ(house->name, "House");
    ASSERT_EQ(house->cost.size(), 1u);
    EXPECT_EQ(house->cost[0].item, world::items::Steel);
    EXPECT_EQ(house->cost[0].count, 100);
    EXPECT_FLOAT_EQ(house->effort, 300.0f);
    const auto* housing = house->blueprint.find<world::Housing>();
    ASSERT_NE(housing, nullptr);
    EXPECT_EQ(housing->workers, 50);
    const auto* power = house->blueprint.find<world::PowerConsumer>();
    ASSERT_NE(power, nullptr);
    EXPECT_FLOAT_EQ(power->demand_kw, 20.0f);
}

TEST(BuildingType, HouseRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::House);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "House");
    EXPECT_EQ(parsed, world::BuildingType::House);
}

namespace {

// Checks a factory catalog entry: cost, effort, crew and power.
void expect_factory_entry(world::BuildingType type, const char* name,
                          const std::vector<world::ItemStack>& cost) {
    const world::Construction* factory = world::find_construction(type);
    ASSERT_NE(factory, nullptr);
    EXPECT_EQ(factory->name, name);
    ASSERT_EQ(factory->cost.size(), cost.size());
    for (std::size_t i = 0; i < cost.size(); ++i) {
        EXPECT_EQ(factory->cost[i].item, cost[i].item);
        EXPECT_EQ(factory->cost[i].count, cost[i].count);
    }
    EXPECT_FLOAT_EQ(factory->effort, 1200.0f);
    ASSERT_NE(factory->blueprint.find<world::Staffing>(), nullptr);
    EXPECT_EQ(factory->blueprint.find<world::Staffing>()->needed, 50);
    ASSERT_NE(factory->blueprint.find<world::PowerConsumer>(), nullptr);
    EXPECT_FLOAT_EQ(factory->blueprint.find<world::PowerConsumer>()->demand_kw, 300.0f);
}

const world::Recipe* recipe_of(world::BuildingType type) {
    const world::Construction* factory = world::find_construction(type);
    if (factory == nullptr || factory->blueprint.find<world::Producer>() == nullptr) {
        return nullptr;
    }
    return &factory->blueprint.find<world::Producer>()->recipe;
}

} // namespace

TEST(ConstructionCatalog, ToolFactoryCosts200SteelAnd100ElectronicsTakes1200WorkNeeds50WorkersOn300Kw) {
    // Given / When / Then
    expect_factory_entry(world::BuildingType::ToolFactory, "Tool Factory",
                         {{world::items::Steel, 200}, {world::items::Electronics, 100}});
}

TEST(ConstructionCatalog, AmmoFactoryCosts200SteelAnd100ToolsTakes1200WorkNeeds50WorkersOn300Kw) {
    // Given / When / Then
    expect_factory_entry(world::BuildingType::AmmoFactory, "Ammo Factory",
                         {{world::items::Steel, 200}, {world::items::Tool, 100}});
}

TEST(ConstructionCatalog, WeaponFactoryCosts200SteelAnd100ToolsTakes1200WorkNeeds50WorkersOn300Kw) {
    // Given / When / Then
    expect_factory_entry(world::BuildingType::WeaponFactory, "Weapon Factory",
                         {{world::items::Steel, 200}, {world::items::Tool, 100}});
}

TEST(ConstructionCatalog, ToolFactoryMakesOneToolFromOneElectronicsAndOneSteel) {
    // Given / When
    const world::Recipe* recipe = recipe_of(world::BuildingType::ToolFactory);

    // Then
    ASSERT_NE(recipe, nullptr);
    EXPECT_EQ(recipe->output, world::items::Tool);
    EXPECT_EQ(recipe->output_count, 1);
    ASSERT_EQ(recipe->inputs.size(), 2u);
    EXPECT_EQ(recipe->inputs[0].item, world::items::Electronics);
    EXPECT_EQ(recipe->inputs[0].count, 1);
    EXPECT_EQ(recipe->inputs[1].item, world::items::Steel);
    EXPECT_EQ(recipe->inputs[1].count, 1);
}

TEST(ConstructionCatalog, AmmoFactoryMakesTwoAmmoFromOneCoalOneIronOreAndOneCopperOre) {
    // Given / When
    const world::Recipe* recipe = recipe_of(world::BuildingType::AmmoFactory);

    // Then
    ASSERT_NE(recipe, nullptr);
    EXPECT_EQ(recipe->output, world::items::Ammo);
    EXPECT_EQ(recipe->output_count, 2);
    ASSERT_EQ(recipe->inputs.size(), 3u);
    EXPECT_EQ(recipe->inputs[0].item, world::items::Coal);
    EXPECT_EQ(recipe->inputs[1].item, world::items::IronOre);
    EXPECT_EQ(recipe->inputs[2].item, world::items::CopperOre);
    for (const auto& input : recipe->inputs) {
        EXPECT_EQ(input.count, 1);
    }
}

TEST(ConstructionCatalog, WeaponFactoryMakesOneWeaponFromOneSteelAndOneTool) {
    // Given / When
    const world::Recipe* recipe = recipe_of(world::BuildingType::WeaponFactory);

    // Then
    ASSERT_NE(recipe, nullptr);
    EXPECT_EQ(recipe->output, world::items::Weapon);
    EXPECT_EQ(recipe->output_count, 1);
    ASSERT_EQ(recipe->inputs.size(), 2u);
    EXPECT_EQ(recipe->inputs[0].item, world::items::Steel);
    EXPECT_EQ(recipe->inputs[1].item, world::items::Tool);
}

TEST(ConstructionCatalog, AmmoFactoryHasGeneralSlotsAndOneSectionPerInput) {
    // Given
    const world::Construction* factory = world::find_construction(world::BuildingType::AmmoFactory);
    ASSERT_NE(factory, nullptr);
    world::SolidInventory inventory = *factory->blueprint.find<world::SolidInventory>();

    // When
    const int coal_left = inventory.add(world::items::Coal, 1000);
    const int iron_left = inventory.add(world::items::IronOre, 1000);
    const int copper_left = inventory.add(world::items::CopperOre, 1000);
    const int ammo_left = inventory.add(world::items::Ammo, 2000);

    // Then: 5 slots per input, 10 general slots for the ammo
    EXPECT_EQ(inventory.capacity(), 25u);
    EXPECT_EQ(coal_left, 500);
    EXPECT_EQ(iron_left, 500);
    EXPECT_EQ(copper_left, 500);
    EXPECT_EQ(ammo_left, 1000);
}

TEST(ConstructionCatalog, ToolAndWeaponFactoriesKeepTheirInputsOutOfTheGeneralSlots) {
    // Given
    const world::Construction* tool_factory =
        world::find_construction(world::BuildingType::ToolFactory);
    const world::Construction* weapon_factory =
        world::find_construction(world::BuildingType::WeaponFactory);
    ASSERT_NE(tool_factory, nullptr);
    ASSERT_NE(weapon_factory, nullptr);
    world::SolidInventory tools = *tool_factory->blueprint.find<world::SolidInventory>();
    world::SolidInventory weapons = *weapon_factory->blueprint.find<world::SolidInventory>();

    // When
    const int electronics_left = tools.add(world::items::Electronics, 1000);
    const int tool_steel_left = tools.add(world::items::Steel, 1000);
    const int tool_left = tools.add(world::items::Tool, 2000);
    const int weapon_steel_left = weapons.add(world::items::Steel, 1000);
    const int weapon_tool_left = weapons.add(world::items::Tool, 1000);
    const int weapon_left = weapons.add(world::items::Weapon, 2000);

    // Then: 5 slots per input, 10 general slots for the output
    EXPECT_EQ(electronics_left, 500);
    EXPECT_EQ(tool_steel_left, 500);
    EXPECT_EQ(tool_left, 1000);
    EXPECT_EQ(weapon_steel_left, 500);
    EXPECT_EQ(weapon_tool_left, 500);
    EXPECT_EQ(weapon_left, 1990);
}

TEST(BuildingType, NewFactoriesRoundTripThroughTheirSaveKeys) {
    // Given
    const std::vector<std::pair<world::BuildingType, std::string_view>> expected{
        {world::BuildingType::ToolFactory, "ToolFactory"},
        {world::BuildingType::AmmoFactory, "AmmoFactory"},
        {world::BuildingType::WeaponFactory, "WeaponFactory"}};

    for (const auto& [type, name] : expected) {
        // When
        const std::string_view key = world::building_type_key(type);

        // Then
        EXPECT_EQ(key, name);
        EXPECT_EQ(world::parse_building_type(key), type);
    }
}

TEST(ConstructionCatalog, BarracksCosts200SteelAnd100ToolsTakes1200WorkAndRecruitsOn200Kw) {
    // Given / When
    const world::Construction* barracks = world::find_construction(world::BuildingType::Barracks);

    // Then
    ASSERT_NE(barracks, nullptr);
    EXPECT_EQ(barracks->name, "Barracks");
    ASSERT_EQ(barracks->cost.size(), 2u);
    EXPECT_EQ(barracks->cost[0].item, world::items::Steel);
    EXPECT_EQ(barracks->cost[0].count, 200);
    EXPECT_EQ(barracks->cost[1].item, world::items::Tool);
    EXPECT_EQ(barracks->cost[1].count, 100);
    EXPECT_FLOAT_EQ(barracks->effort, 1200.0f);
    EXPECT_NE(barracks->blueprint.find<world::Recruiter>(), nullptr);
    const auto* power = barracks->blueprint.find<world::PowerConsumer>();
    ASSERT_NE(power, nullptr);
    EXPECT_FLOAT_EQ(power->demand_kw, 200.0f);
}

TEST(ConstructionCatalog, BarracksIsASupplyHolding5WeaponsAnd5AmmoStacksOnly) {
    // Given
    const world::Construction* barracks = world::find_construction(world::BuildingType::Barracks);
    ASSERT_NE(barracks, nullptr);
    ASSERT_NE(barracks->blueprint.find<world::SolidInventory>(), nullptr);
    world::SolidInventory inventory = *barracks->blueprint.find<world::SolidInventory>();

    // When
    const int weapons_left = inventory.add(world::items::Weapon, 600);
    const int ammo_left = inventory.add(world::items::Ammo, 600);
    const int steel_left = inventory.add(world::items::Steel, 10);

    // Then
    EXPECT_NE(barracks->blueprint.find<world::Supply>(), nullptr);
    EXPECT_EQ(weapons_left, 595);
    EXPECT_EQ(ammo_left, 100);
    EXPECT_EQ(steel_left, 10);
}

TEST(BuildingType, BarracksRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::Barracks);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "Barracks");
    EXPECT_EQ(parsed, world::BuildingType::Barracks);
}

TEST(ConstructionCatalog, TrainingTargetIsAFree1x1MilitaryBuildingThatCanBeShot) {
    // Given / When
    const world::Construction* target =
        world::find_construction(world::BuildingType::TrainingTarget);

    // Then
    ASSERT_NE(target, nullptr);
    EXPECT_EQ(target->name, "Training Target");
    EXPECT_TRUE(target->cost.empty());
    EXPECT_FLOAT_EQ(target->effort, 0.0f);
    EXPECT_EQ(target->size.width, 1);
    EXPECT_EQ(target->size.height, 1);
    EXPECT_NE(target->blueprint.find<world::Health>(), nullptr);
    EXPECT_NE(target->blueprint.find<world::TrainingTarget>(), nullptr);
    EXPECT_EQ(target->blueprint.find<world::PowerConsumer>(), nullptr);
}

TEST(BuildingType, TrainingTargetRoundTripsThroughItsSaveKey) {
    // Given
    const std::string_view key = world::building_type_key(world::BuildingType::TrainingTarget);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "TrainingTarget");
    EXPECT_EQ(parsed, world::BuildingType::TrainingTarget);
}

TEST(ConstructionCatalog, OilPumpIsAMineCosting100SteelAnd100IronOreOn100KwWithALiquidTank) {
    // Given
    const auto& mines = world::construction_catalog()[1];

    // When
    const world::Construction* pump = world::find_construction(world::BuildingType::OilPump);

    // Then
    ASSERT_NE(pump, nullptr);
    EXPECT_EQ(mines.category, world::ConstructionCategory::Mine);
    EXPECT_TRUE(std::any_of(mines.constructions.begin(), mines.constructions.end(),
                            [](const auto& c) { return c.type == world::BuildingType::OilPump; }));
    ASSERT_EQ(pump->cost.size(), 2u);
    EXPECT_EQ(pump->cost[0].item, world::items::Steel);
    EXPECT_EQ(pump->cost[0].count, 100);
    EXPECT_EQ(pump->cost[1].item, world::items::IronOre);
    EXPECT_EQ(pump->cost[1].count, 100);
    ASSERT_NE(pump->blueprint.find<world::PowerConsumer>(), nullptr);
    EXPECT_FLOAT_EQ(pump->blueprint.find<world::PowerConsumer>()->demand_kw, 100.0f);
    ASSERT_NE(pump->blueprint.find<world::Producer>(), nullptr);
    EXPECT_FLOAT_EQ(pump->blueprint.find<world::Producer>()->recipe.rate, 10.0f);
    ASSERT_NE(pump->blueprint.find<world::LiquidInventory>(), nullptr);
    EXPECT_EQ(pump->blueprint.find<world::LiquidInventory>()->capacity(), 1000);
    EXPECT_EQ(pump->blueprint.find<world::SolidInventory>(), nullptr);
}

TEST(BuildingType, OilPumpRoundTripsThroughItsSaveKey) {
    // Given
    const auto key = world::building_type_key(world::BuildingType::OilPump);

    // When
    const auto parsed = world::parse_building_type(key);

    // Then
    EXPECT_EQ(key, "OilPump");
    EXPECT_EQ(parsed, world::BuildingType::OilPump);
}
