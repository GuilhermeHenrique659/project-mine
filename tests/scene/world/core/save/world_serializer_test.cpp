#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "logging/logger.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/construction/build_rules.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/save/codecs.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/persistent.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/save/world_serializer.hpp"

namespace {

struct Fixture {
    world::EntityFactory factory = world::make_entity_factory();
    world::WorldSerializer serializer = world::make_world_serializer(factory);
    core::Registry registry;

    core::EntityId spawn_player(world::WorldPoint at) {
        const core::EntityId id = *factory.spawn(registry, "character", "Player");
        registry.get<world::Position>(id)->point = at;
        registry.add(id, world::PlayerControlled{});
        return id;
    }
};

game::EntityRecord player_record(std::map<std::string, std::string> fields) {
    return {"character", "Player", std::move(fields)};
}

} // namespace

TEST(WorldSerializer, SnapshotOfAMovingPlayerWritesItsPositionAndIdle) {
    // Given
    Fixture f;
    const core::EntityId id = f.spawn_player({3.25f, -1.5f});
    f.registry.get<world::Character>(id)->state = world::CharacterState::Moving;

    // When
    const auto records = f.serializer.snapshot(f.registry);

    // Then
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].archetype, "character");
    EXPECT_EQ(records[0].label, "Player");
    EXPECT_EQ(records[0].fields.at("position.x"), "3.25");
    EXPECT_EQ(records[0].fields.at("position.y"), "-1.5");
    EXPECT_EQ(records[0].fields.at("state"), "Idle");
    EXPECT_EQ(records[0].fields.at("player"), "1");
}

TEST(WorldSerializer, RunningIsAlsoSavedAsIdle) {
    // Given
    Fixture f;
    const core::EntityId id = f.spawn_player({0.0f, 0.0f});
    f.registry.get<world::Character>(id)->state = world::CharacterState::Running;

    // When
    const auto records = f.serializer.snapshot(f.registry);

    // Then
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].fields.at("state"), "Idle");
}

TEST(WorldSerializer, EntitiesWithoutPersistentAreNotSaved) {
    // Given
    Fixture f;
    f.spawn_player({1.0f, 1.0f});
    const core::EntityId transient = f.registry.create("Spark");
    f.registry.add(transient, world::Position{{5.0f, 5.0f}});

    // When
    const auto records = f.serializer.snapshot(f.registry);

    // Then
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].label, "Player");
}

TEST(WorldSerializer, FractionalPositionsRoundTripExactly) {
    // Given
    Fixture f;
    f.spawn_player({0.1f, -123.456f});
    const auto records = f.serializer.snapshot(f.registry);
    core::Registry loaded;

    // When
    f.serializer.restore(loaded, records);

    // Then
    ASSERT_EQ(loaded.entities().size(), 1u);
    const core::EntityId id = loaded.entities()[0].id;
    EXPECT_EQ(loaded.get<world::Position>(id)->point.x, 0.1f);
    EXPECT_EQ(loaded.get<world::Position>(id)->point.y, -123.456f);
}

TEST(WorldSerializer, RestoreRecreatesPositionIdleStateAndPlayerMarker) {
    // Given
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{player_record(
        {{"position.x", "4"}, {"position.y", "-2"}, {"state", "Idle"}, {"player", "1"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then
    ASSERT_EQ(f.registry.entities().size(), 1u);
    const core::EntityId id = f.registry.entities()[0].id;
    EXPECT_EQ(f.registry.find(id)->label, "Player");
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.x, 4.0f);
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.y, -2.0f);
    EXPECT_EQ(f.registry.get<world::Character>(id)->state, world::CharacterState::Idle);
    EXPECT_TRUE(f.registry.has<world::PlayerControlled>(id));
    EXPECT_TRUE(f.registry.has<world::Persistent>(id));
}

TEST(WorldSerializer, RestoreSkipsAnUnknownArchetypeAndKeepsTheRest) {
    // Given
    Fixture f;
    logging::set_sink([](logging::Level, const std::string&) {});
    const auto records = std::vector<game::EntityRecord>{
        {"dragon", "Smaug", {}}, player_record({{"position.x", "2"}, {"position.y", "2"}})};

    // When
    f.serializer.restore(f.registry, records);
    logging::set_sink({});

    // Then
    ASSERT_EQ(f.registry.entities().size(), 1u);
    EXPECT_EQ(f.registry.entities()[0].label, "Player");
}

TEST(WorldSerializer, ABadPositionValueKeepsTheDefault) {
    // Given
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{
        player_record({{"position.x", "abc"}, {"position.y", "5"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then: neither coordinate is applied when one is invalid
    const core::EntityId id = f.registry.entities()[0].id;
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.x, 0.0f);
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.y, 0.0f);
}

TEST(WorldSerializer, AnUnknownStateRestoresAsIdle) {
    // Given
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{player_record({{"state", "Flying"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then
    const core::EntityId id = f.registry.entities()[0].id;
    EXPECT_EQ(f.registry.get<world::Character>(id)->state, world::CharacterState::Idle);
}

TEST(WorldSerializer, ASavedMovingStateRestoresAsIdle) {
    // Given: movement intent is not saved, so the character is standing after a load
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{player_record({{"state", "Moving"}}),
                                                         player_record({{"state", "Running"}})};

    // When
    f.serializer.restore(f.registry, records);

    // Then
    for (const core::Entity& entity : f.registry.entities()) {
        EXPECT_EQ(f.registry.get<world::Character>(entity.id)->state, world::CharacterState::Idle);
    }
}

TEST(WorldSerializer, SnapshotWritesEachFilledInventorySlotWithItsIndex) {
    // Given
    Fixture f;
    const core::EntityId id = f.spawn_player({0.0f, 0.0f});
    world::Inventory& inventory = *f.registry.get<world::Inventory>(id);
    inventory.set_slot(0, world::ItemStack{world::items::IronOre, 100});
    inventory.set_slot(3, world::ItemStack{world::items::Coal, 7});

    // When
    const auto records = f.serializer.snapshot(f.registry);

    // Then
    ASSERT_EQ(records.size(), 1u);
    const auto& fields = records[0].fields;
    EXPECT_EQ(fields.at("inventory.0"), "IronOre:100");
    EXPECT_EQ(fields.at("inventory.3"), "Coal:7");
    EXPECT_EQ(fields.count("inventory.1"), 0u);
}

TEST(WorldSerializer, SteelInAnInventorySurvivesASaveAndLoad) {
    // Given
    Fixture saved;
    const core::EntityId id = saved.spawn_player({0.0f, 0.0f});
    saved.registry.get<world::Inventory>(id)->set_slot(
        0, world::ItemStack{world::items::Steel, 12});
    const auto records = saved.serializer.snapshot(saved.registry);
    Fixture loaded;

    // When
    loaded.serializer.restore(loaded.registry, records);

    // Then
    EXPECT_EQ(records[0].fields.at("inventory.0"), "Steel:12");
    const auto& slots =
        loaded.registry.get<world::Inventory>(loaded.registry.entities()[0].id)->slots();
    ASSERT_TRUE(slots[0].has_value());
    EXPECT_EQ(slots[0]->item, world::items::Steel);
    EXPECT_EQ(slots[0]->count, 12);
}

TEST(WorldSerializer, ElectronicsInAnInventorySurviveASaveAndLoad) {
    // Given
    Fixture saved;
    const core::EntityId id = saved.spawn_player({0.0f, 0.0f});
    saved.registry.get<world::Inventory>(id)->set_slot(
        0, world::ItemStack{world::items::Electronics, 7});
    const auto records = saved.serializer.snapshot(saved.registry);
    Fixture loaded;

    // When
    loaded.serializer.restore(loaded.registry, records);

    // Then
    EXPECT_EQ(records[0].fields.at("inventory.0"), "Electronics:7");
    const auto& slots =
        loaded.registry.get<world::Inventory>(loaded.registry.entities()[0].id)->slots();
    ASSERT_TRUE(slots[0].has_value());
    EXPECT_EQ(slots[0]->item, world::items::Electronics);
    EXPECT_EQ(slots[0]->count, 7);
}

TEST(WorldSerializer, InventoryRoundTripKeepsEveryStackInItsSlot) {
    // Given
    Fixture saved;
    const core::EntityId id = saved.spawn_player({0.0f, 0.0f});
    world::Inventory& inventory = *saved.registry.get<world::Inventory>(id);
    inventory.set_slot(2, world::ItemStack{world::items::CopperOre, 42});
    inventory.set_slot(39, world::ItemStack{world::items::IronOre, 1});
    const auto records = saved.serializer.snapshot(saved.registry);
    Fixture loaded;

    // When
    loaded.serializer.restore(loaded.registry, records);

    // Then
    const auto& slots =
        loaded.registry.get<world::Inventory>(loaded.registry.entities()[0].id)->slots();
    ASSERT_EQ(slots.size(), 40u);
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (i == 2 || i == 39) {
            continue;
        }
        EXPECT_FALSE(slots[i].has_value()) << "slot " << i;
    }
    ASSERT_TRUE(slots[2].has_value());
    EXPECT_EQ(slots[2]->item, world::items::CopperOre);
    EXPECT_EQ(slots[2]->count, 42);
    ASSERT_TRUE(slots[39].has_value());
    EXPECT_EQ(slots[39]->item, world::items::IronOre);
    EXPECT_EQ(slots[39]->count, 1);
}

TEST(WorldSerializer, BadInventoryEntriesAreSkippedAndTheValidOnesKept) {
    // Given
    Fixture f;
    const auto records = std::vector<game::EntityRecord>{player_record({
        {"inventory.0", "Diamond:5"},   // unknown item
        {"inventory.40", "Coal:5"},     // outside the 40 slots
        {"inventory.-1", "Coal:5"},     // not a slot index
        {"inventory.1", "Coal:0"},      // empty stack
        {"inventory.2", "Coal:101"},    // above max stack
        {"inventory.3", "Coal"},        // no count
        {"inventory.4", "IronOre:x"},   // bad count
        {"inventory.5", "Coal:100"},    // valid
    })};

    // When
    f.serializer.restore(f.registry, records);

    // Then
    const world::Inventory& inventory =
        *f.registry.get<world::Inventory>(f.registry.entities()[0].id);
    EXPECT_EQ(inventory.count(world::items::Coal), 100);
    EXPECT_EQ(inventory.count(world::items::IronOre), 0);
    EXPECT_EQ(inventory.free_slots(), 39u);
    ASSERT_TRUE(inventory.slots()[5].has_value());
}

TEST(WorldSerializer, AHeadquartersRoundTripsOnItsTile) {
    // Given
    Fixture saved;
    const core::EntityId id = *saved.factory.spawn(saved.registry, world::BuildingArchetype,
                                                   "Headquarters");
    world::make_building(saved.registry, id,
                          *world::find_construction(world::BuildingType::Headquarters), {-3, 7});
    const auto records = saved.serializer.snapshot(saved.registry);

    // When
    Fixture loaded;
    loaded.serializer.restore(loaded.registry, records);

    // Then
    ASSERT_EQ(loaded.registry.entities().size(), 1u);
    const core::EntityId hq = loaded.registry.entities().front().id;
    const world::Building* building = loaded.registry.get<world::Building>(hq);
    ASSERT_NE(building, nullptr);
    EXPECT_EQ(building->type, world::BuildingType::Headquarters);
    EXPECT_EQ(building->tile, (world::TileCoord{-3, 7}));
    EXPECT_FLOAT_EQ(loaded.registry.get<world::Position>(hq)->point.x, -0.5f); // centre of its 5x5 footprint
    EXPECT_FALSE(loaded.registry.has<world::ConstructionSite>(hq));
    EXPECT_TRUE(world::has_headquarters(loaded.registry));
}

TEST(ConstructionSiteCodec, KeepsTheProgressOfAHalfBuiltSite) {
    // Given
    core::Registry registry;
    const world::ConstructionSiteCodec codec;
    const core::EntityId saved = registry.create("site");
    registry.add(saved, world::Building{});
    registry.add(saved, world::ConstructionSite{1.5f, 4.0f});
    world::Fields fields;
    codec.write(registry, saved, fields);
    const core::EntityId loaded = registry.create("site");
    registry.add(loaded, world::Building{});
    registry.add(loaded, world::ConstructionSite{0.0f, 4.0f});

    // When
    codec.read(registry, loaded, fields);

    // Then
    ASSERT_TRUE(registry.has<world::ConstructionSite>(loaded));
    EXPECT_FLOAT_EQ(registry.get<world::ConstructionSite>(loaded)->progress, 1.5f);
}

TEST(ConstructionSiteCodec, ABuildingSavedFinishedLoadsWithoutASite) {
    // Given
    core::Registry registry;
    const world::ConstructionSiteCodec codec;
    const core::EntityId loaded = registry.create("building");
    registry.add(loaded, world::Building{});
    registry.add(loaded, world::ConstructionSite{0.0f, 4.0f});

    // When
    codec.read(registry, loaded, world::Fields{{"building.type", "Headquarters"}});

    // Then
    EXPECT_FALSE(registry.has<world::ConstructionSite>(loaded));
}

TEST(WorldSerializer, AMineKeepsItsInventoryThroughASave) {
    // Given
    Fixture saved;
    const core::EntityId id = *saved.factory.spawn(saved.registry, world::BuildingArchetype, "Mine");
    world::make_building(saved.registry, id, *world::find_construction(world::BuildingType::Mine),
                         {30, 2});
    saved.registry.get<world::Inventory>(id)->set_slot(2, world::ItemStack{world::items::Coal, 40});
    const auto records = saved.serializer.snapshot(saved.registry);

    // When
    Fixture loaded;
    loaded.serializer.restore(loaded.registry, records);

    // Then
    const core::EntityId mine = loaded.registry.entities().front().id;
    const world::Inventory* inventory = loaded.registry.get<world::Inventory>(mine);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->capacity(), 10u);
    ASSERT_TRUE(inventory->slots()[2].has_value());
    EXPECT_EQ(inventory->slots()[2]->count, 40);
}

TEST(WorldSerializer, ABuildingIsPlacedOnItsFootprintWhateverPositionWasSaved) {
    // Given: a 5x5 headquarters saved by an older version with its origin tile's centre
    Fixture f;
    const std::vector<game::EntityRecord> records{
        {"building", "Headquarters",
         {{"building.type", "Headquarters"}, {"building.x", "0"}, {"building.y", "0"},
          {"position.x", "0.5"}, {"position.y", "0.5"}}}};

    // When
    f.serializer.restore(f.registry, records);

    // Then: it stands at the centre of the tiles it covers
    ASSERT_EQ(f.registry.entities().size(), 1u);
    const core::EntityId id = f.registry.entities()[0].id;
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.x, 2.5f);
    EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.y, 2.5f);
}
