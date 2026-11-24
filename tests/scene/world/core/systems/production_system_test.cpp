#include <gtest/gtest.h>

#include <optional>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/liquid_inventory.hpp"
#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/systems/production_system.hpp"

namespace {

std::optional<world::TileCoord> find_tile(const world::Terrain& terrain, world::TileType type) {
    for (int x = -300; x < 300; ++x) {
        for (int y = -300; y < 300; ++y) {
            if (terrain.tile_at({x, y}) == type) {
                return world::TileCoord{x, y};
            }
        }
    }
    return std::nullopt;
}

struct Fixture {
    core::Registry registry;
    world::Terrain terrain{7};
    world::ProductionSystem system{registry, terrain};

    // A finished catalog mine on the first tile of `ore`.
    core::EntityId mine_on(world::TileType ore) {
        const auto tile = find_tile(terrain, ore);
        EXPECT_TRUE(tile);
        const core::EntityId id = registry.create("Mine");
        world::make_building(registry, id, *world::find_construction(world::BuildingType::Mine),
                             tile.value_or(world::TileCoord{}));
        registry.remove<world::ConstructionSite>(id);
        return id;
    }

    // `count` workers assigned to `building`, standing `offset` tiles from it.
    void staff(core::EntityId building, int count, float offset = 0.0f) {
        const world::WorldPoint at = registry.get<world::Position>(building)->point;
        for (int i = 0; i < count; ++i) {
            const core::EntityId id = registry.create("Worker");
            registry.add(id, world::Worker{world::WorkerState::Working, building});
            registry.add(id, world::Position{{at.x + offset, at.y}});
        }
    }

    // A fully staffed catalog mine on the first tile of `ore`.
    core::EntityId staffed_mine_on(world::TileType ore) {
        const core::EntityId id = mine_on(ore);
        staff(id, world::find_construction(world::BuildingType::Mine)
                      ->blueprint.find<world::Staffing>()->needed);
        return id;
    }

    // A finished building with its own recipe and inventory on grass.
    core::EntityId producer(world::Recipe recipe, std::size_t slots) {
        const core::EntityId id = registry.create("Smelter");
        registry.add(id, world::Building{world::BuildingType::Mine, {1, 1}});
        registry.add(id, world::SolidInventory{slots});
        registry.add(id, world::Producer{recipe});
        registry.add(id, world::BuildingStatus{});
        return id;
    }

    world::BuildingStatus status(core::EntityId id) const {
        const world::BuildingStatus* status = registry.get<world::BuildingStatus>(id);
        EXPECT_NE(status, nullptr);
        return status != nullptr ? *status : world::BuildingStatus{};
    }

    int count(core::EntityId id, world::Item item) const {
        return registry.get<world::SolidInventory>(id)->count(item);
    }
};

} // namespace

TEST(ProductionSystem, TheMineRecipeNeedsNoInputAndYieldsTheGroundAtHalfAnItemPerSecond) {
    // Given / When
    const auto* producer =
        world::find_construction(world::BuildingType::Mine)->blueprint.find<world::Producer>();

    // Then
    ASSERT_NE(producer, nullptr);
    const world::Recipe* recipe = &producer->recipe;
    EXPECT_TRUE(recipe->inputs.empty());
    EXPECT_FALSE(recipe->output.has_value());
    EXPECT_FLOAT_EQ(recipe->rate, 4.0f);
}

TEST(ProductionSystem, AMineNeedsTenWorkersForItsFullRate) {
    // Given / When
    const auto* staffing =
        world::find_construction(world::BuildingType::Mine)->blueprint.find<world::Staffing>();

    // Then
    ASSERT_NE(staffing, nullptr);
    EXPECT_EQ(staffing->needed, 10);
}

TEST(ProductionSystem, AMineWithHalfItsWorkersProducesAtHalfTheRate) {
    // Given
    Fixture f;
    const core::EntityId mine = f.mine_on(world::TileType::IronOre);
    f.staff(mine, 5);

    // When: 4/s at half rate = one ore every 0.5 s
    f.system.update(0.45f);
    const int before = f.count(mine, world::items::IronOre);
    f.system.update(0.1f);

    // Then
    EXPECT_EQ(before, 0);
    EXPECT_EQ(f.count(mine, world::items::IronOre), 1);
    EXPECT_EQ(f.status(mine).state, world::BuildingState::Working);
}

TEST(ProductionSystem, AMineWithoutWorkersIsIdleForLackOfWorkersAndProducesNothing) {
    // Given
    Fixture f;
    const core::EntityId mine = f.mine_on(world::TileType::IronOre);

    // When
    f.system.update(10.0f);

    // Then
    EXPECT_EQ(f.count(mine, world::items::IronOre), 0);
    EXPECT_EQ(f.status(mine).state, world::BuildingState::Idle);
    EXPECT_EQ(f.status(mine).reason, world::IdleReason::NoWorkers);
}

TEST(ProductionSystem, WorkersStillWalkingToTheBuildingDoNotCount) {
    // Given: assigned, but 5 tiles away (reach is 2)
    Fixture f;
    const core::EntityId mine = f.mine_on(world::TileType::IronOre);
    f.staff(mine, 10, 5.0f);

    // When
    f.system.update(10.0f);

    // Then
    EXPECT_EQ(f.count(mine, world::items::IronOre), 0);
    EXPECT_EQ(f.status(mine).reason, world::IdleReason::NoWorkers);
}

TEST(ProductionSystem, AMineProducesTheOreUnderItAtItsRate) {
    // Given
    Fixture f;
    const core::EntityId iron = f.staffed_mine_on(world::TileType::IronOre);
    const core::EntityId coal = f.staffed_mine_on(world::TileType::Coal);

    // When
    f.system.update(0.5f);

    // Then
    EXPECT_EQ(f.count(iron, world::items::IronOre), 2);
    EXPECT_EQ(f.count(coal, world::items::Coal), 2);
}

TEST(ProductionSystem, PartialProgressCarriesOverToTheNextFrame) {
    // Given
    Fixture f;
    const core::EntityId mine = f.staffed_mine_on(world::TileType::CopperOre);

    // When
    f.system.update(0.15f);
    const int after_first = f.count(mine, world::items::CopperOre);
    f.system.update(0.2f);

    // Then
    EXPECT_EQ(after_first, 0);
    EXPECT_EQ(f.count(mine, world::items::CopperOre), 1);
}

TEST(ProductionSystem, AConstructionSiteProducesNothing) {
    // Given
    Fixture f;
    const core::EntityId mine = f.staffed_mine_on(world::TileType::IronOre);
    f.registry.add(mine, world::ConstructionSite{0.0f, 300.0f});

    // When
    f.system.update(10.0f);

    // Then
    EXPECT_EQ(f.count(mine, world::items::IronOre), 0);
}

TEST(ProductionSystem, AFullInventoryWaitsWithAtMostOneItemOfProgress) {
    // Given: one slot, already full of iron ore
    Fixture f;
    const core::EntityId id = f.producer({{}, world::items::IronOre, 1.0f}, 1);
    f.registry.get<world::SolidInventory>(id)->add(world::items::IronOre, 100);

    // When
    f.system.update(5.0f);
    f.registry.get<world::SolidInventory>(id)->remove(world::items::IronOre, 10);
    f.system.update(0.0f);

    // Then: the waiting item comes out as soon as there is room, the rest was not banked
    EXPECT_EQ(f.count(id, world::items::IronOre), 91);
}

TEST(ProductionSystem, AnInputIsConsumedForEachOutputAndItsLackStopsProduction) {
    // Given: 2 iron ore -> 1 coal, with iron for one output only
    Fixture f;
    const core::EntityId id =
        f.producer({{world::ItemStack{world::items::IronOre, 2}}, world::items::Coal, 1.0f}, 4);
    f.registry.get<world::SolidInventory>(id)->add(world::items::IronOre, 3);

    // When
    f.system.update(3.0f);

    // Then
    EXPECT_EQ(f.count(id, world::items::Coal), 1);
    EXPECT_EQ(f.count(id, world::items::IronOre), 1);
}

TEST(ProductionSystem, AnUnpoweredBuildingIsIdleForLackOfPowerAndProducesNothing) {
    // Given
    Fixture f;
    const core::EntityId mine = f.staffed_mine_on(world::TileType::IronOre);
    f.registry.get<world::PowerConsumer>(mine)->powered = false;

    // When
    f.system.update(4.0f);

    // Then
    EXPECT_EQ(f.count(mine, world::items::IronOre), 0);
    EXPECT_EQ(f.status(mine).state, world::BuildingState::Idle);
    EXPECT_EQ(f.status(mine).reason, world::IdleReason::NoPower);
}

TEST(ProductionSystem, APoweredProducingBuildingIsWorking) {
    // Given: a mine idle last frame for lack of power, powered again
    Fixture f;
    const core::EntityId mine = f.staffed_mine_on(world::TileType::IronOre);
    f.registry.get<world::PowerConsumer>(mine)->powered = false;
    f.system.update(1.0f);
    f.registry.get<world::PowerConsumer>(mine)->powered = true;

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.status(mine).state, world::BuildingState::Working);
    EXPECT_EQ(f.status(mine).reason, world::IdleReason::None);
}

TEST(ProductionSystem, ABuildingWithoutItsInputIsIdleForMissingInput) {
    // Given: 2 iron ore -> 1 coal, with no iron
    Fixture f;
    const core::EntityId id =
        f.producer({{world::ItemStack{world::items::IronOre, 2}}, world::items::Coal, 1.0f}, 4);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.status(id).state, world::BuildingState::Idle);
    EXPECT_EQ(f.status(id).reason, world::IdleReason::MissingInput);
}

TEST(ProductionSystem, ABuildingWhoseOutputDoesNotFitIsIdleForFullOutput) {
    // Given: one slot, already full of iron ore
    Fixture f;
    const core::EntityId id = f.producer({{}, world::items::IronOre, 1.0f}, 1);
    f.registry.get<world::SolidInventory>(id)->add(world::items::IronOre, 100);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.status(id).state, world::BuildingState::Idle);
    EXPECT_EQ(f.status(id).reason, world::IdleReason::OutputFull);
}

TEST(ProductionSystem, AMineOffOreIsIdleForLackOfResource) {
    // Given: a catalog mine on grass
    Fixture f;
    const core::EntityId mine = f.registry.create("Mine");
    world::make_building(f.registry, mine, *world::find_construction(world::BuildingType::Mine),
                         {1, 1});
    f.registry.remove<world::ConstructionSite>(mine);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.status(mine).state, world::BuildingState::Idle);
    EXPECT_EQ(f.status(mine).reason, world::IdleReason::NoResource);
}

TEST(ProductionSystem, ARecipeWithSeveralInputsNeedsAllOfThemForEachOutput) {
    // Given: 2 iron ore + 2 coal -> 1 copper ore, with coal for one output only
    Fixture f;
    const core::EntityId id = f.producer(
        {{world::ItemStack{world::items::IronOre, 2}, world::ItemStack{world::items::Coal, 2}},
         world::items::CopperOre,
         1.0f},
        4);
    f.registry.get<world::SolidInventory>(id)->add(world::items::IronOre, 6);
    f.registry.get<world::SolidInventory>(id)->add(world::items::Coal, 3);

    // When
    f.system.update(3.0f);

    // Then
    EXPECT_EQ(f.count(id, world::items::CopperOre), 1);
    EXPECT_EQ(f.count(id, world::items::IronOre), 4);
    EXPECT_EQ(f.count(id, world::items::Coal), 1);
    EXPECT_EQ(f.status(id).reason, world::IdleReason::MissingInput);
}

TEST(ProductionSystem, AStaffedSteelworksTurnsTwoCoalAndTwoIronOreIntoSteelTwicePerSecond) {
    // Given
    Fixture f;
    const world::Construction* steelworks =
        world::find_construction(world::BuildingType::Steelworks);
    ASSERT_NE(steelworks, nullptr);
    const core::EntityId id = f.registry.create("Steelworks");
    world::make_building(f.registry, id, *steelworks, {1, 1});
    f.registry.remove<world::ConstructionSite>(id);
    f.staff(id, steelworks->blueprint.find<world::Staffing>()->needed);
    f.registry.get<world::SolidInventory>(id)->add(world::items::IronOre, 10);
    f.registry.get<world::SolidInventory>(id)->add(world::items::Coal, 10);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.count(id, world::items::Steel), 2);
    EXPECT_EQ(f.count(id, world::items::IronOre), 6);
    EXPECT_EQ(f.count(id, world::items::Coal), 6);
}

TEST(ProductionSystem, AStaffedElectronicFactoryTurnsCopperAndIronOreIntoElectronicsTwicePerSecond) {
    // Given
    Fixture f;
    const world::Construction* factory =
        world::find_construction(world::BuildingType::ElectronicFactory);
    ASSERT_NE(factory, nullptr);
    const core::EntityId id = f.registry.create("Electronic Factory");
    world::make_building(f.registry, id, *factory, {1, 1});
    f.registry.remove<world::ConstructionSite>(id);
    f.staff(id, factory->blueprint.find<world::Staffing>()->needed);
    f.registry.get<world::SolidInventory>(id)->add(world::items::CopperOre, 10);
    f.registry.get<world::SolidInventory>(id)->add(world::items::IronOre, 10);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.count(id, world::items::Electronics), 2);
    EXPECT_EQ(f.count(id, world::items::CopperOre), 6);
    EXPECT_EQ(f.count(id, world::items::IronOre), 6);
}

TEST(ProductionSystem, AStaffedAmmoFactoryMakesFourAmmoPerSecondFromTwoOfEachInput) {
    // Given
    Fixture f;
    const world::Construction* factory = world::find_construction(world::BuildingType::AmmoFactory);
    ASSERT_NE(factory, nullptr);
    const core::EntityId id = f.registry.create("Ammo Factory");
    world::make_building(f.registry, id, *factory, {1, 1});
    f.registry.remove<world::ConstructionSite>(id);
    f.staff(id, factory->blueprint.find<world::Staffing>()->needed);
    f.registry.get<world::SolidInventory>(id)->add(world::items::Coal, 10);
    f.registry.get<world::SolidInventory>(id)->add(world::items::IronOre, 10);
    f.registry.get<world::SolidInventory>(id)->add(world::items::CopperOre, 10);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.count(id, world::items::Ammo), 4);
    EXPECT_EQ(f.count(id, world::items::Coal), 8);
    EXPECT_EQ(f.count(id, world::items::IronOre), 8);
    EXPECT_EQ(f.count(id, world::items::CopperOre), 8);
}

TEST(ProductionSystem, AStaffedWeaponFactoryTurnsSteelAndToolsIntoTwoWeaponsPerSecond) {
    // Given
    Fixture f;
    const world::Construction* factory =
        world::find_construction(world::BuildingType::WeaponFactory);
    ASSERT_NE(factory, nullptr);
    const core::EntityId id = f.registry.create("Weapon Factory");
    world::make_building(f.registry, id, *factory, {1, 1});
    f.registry.remove<world::ConstructionSite>(id);
    f.staff(id, factory->blueprint.find<world::Staffing>()->needed);
    f.registry.get<world::SolidInventory>(id)->add(world::items::Steel, 10);
    f.registry.get<world::SolidInventory>(id)->add(world::items::Tool, 10);

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(f.count(id, world::items::Weapon), 2);
    EXPECT_EQ(f.count(id, world::items::Steel), 8);
    EXPECT_EQ(f.count(id, world::items::Tool), 8);
}

TEST(ProductionSystem, AStaffedEngineFactoryMakesOneEngineFromSteelAndToolsPerMinute) {
    // Given
    Fixture f;
    const world::Construction* factory =
        world::find_construction(world::BuildingType::EngineFactory);
    ASSERT_NE(factory, nullptr);
    const core::EntityId id = f.registry.create("Engine Factory");
    world::make_building(f.registry, id, *factory, {1, 1});
    f.registry.remove<world::ConstructionSite>(id);
    f.staff(id, factory->blueprint.find<world::Staffing>()->needed);
    f.registry.get<world::SolidInventory>(id)->add(world::items::Steel, 10);
    f.registry.get<world::SolidInventory>(id)->add(world::items::Tool, 10);

    // When: 30 s is half an engine, 60 s a whole one
    for (int second = 0; second < 30; ++second) {
        f.system.update(1.0f);
    }
    const int after_half_minute = f.count(id, world::items::Engine);
    for (int second = 0; second < 30; ++second) {
        f.system.update(1.0f);
    }

    // Then
    EXPECT_EQ(after_half_minute, 0);
    EXPECT_EQ(f.count(id, world::items::Engine), 1);
    EXPECT_EQ(f.count(id, world::items::Steel), 9);
    EXPECT_EQ(f.count(id, world::items::Tool), 9);
}

namespace {

// A finished catalog oil pump on the first crude oil tile.
core::EntityId oil_pump(Fixture& f) {
    const auto tile = find_tile(f.terrain, world::TileType::CrudeOil);
    EXPECT_TRUE(tile);
    const core::EntityId id = f.registry.create("Oil Pump");
    world::make_building(f.registry, id, *world::find_construction(world::BuildingType::OilPump),
                         tile.value_or(world::TileCoord{}));
    f.registry.remove<world::ConstructionSite>(id);
    return id;
}

} // namespace

TEST(ProductionSystem, AnOilPumpOnCrudeOilPumpsTenOilPerSecondIntoItsTankWithoutWorkers) {
    // Given
    Fixture f;
    const core::EntityId pump = oil_pump(f);

    // When
    f.system.update(1.0f);

    // Then
    const world::LiquidInventory* tank = f.registry.get<world::LiquidInventory>(pump);
    ASSERT_NE(tank, nullptr);
    EXPECT_EQ(tank->count(world::items::Oil), 10);
    EXPECT_EQ(f.status(pump).state, world::BuildingState::Working);
}

TEST(ProductionSystem, AnOilPumpWithAFullTankIsIdleForAFullOutput) {
    // Given
    Fixture f;
    const core::EntityId pump = oil_pump(f);
    world::LiquidInventory* tank = f.registry.get<world::LiquidInventory>(pump);
    ASSERT_NE(tank, nullptr);
    tank->add(world::items::Oil, tank->capacity());

    // When
    f.system.update(1.0f);

    // Then
    EXPECT_EQ(tank->count(world::items::Oil), tank->capacity());
    EXPECT_EQ(f.status(pump).reason, world::IdleReason::OutputFull);
}
