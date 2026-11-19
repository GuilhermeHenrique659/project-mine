#include <gtest/gtest.h>

#include <vector>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/entities/entity_factory.hpp"
#include "scene/world/core/entities/soldier.hpp"
#include "scene/world/core/events/military_events.hpp"
#include "scene/world/core/occupancy.hpp"
#include "scene/world/core/systems/recruit_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace {

constexpr world::TileCoord BarracksTile{4, 2};

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::EntityFactory factory = world::make_entity_factory();
    world::Terrain terrain{7}; // dry land around the barracks
    world::RecruitSystem system{registry, bus, factory, terrain};

    core::EntityId build(world::BuildingType type, bool finished,
                         world::TileCoord tile = BarracksTile) {
        const core::EntityId id = *factory.spawn(registry, world::BuildingArchetype, "Building");
        world::make_building(registry, id, *world::find_construction(type), tile);
        if (finished) {
            registry.remove<world::ConstructionSite>(id);
        }
        return id;
    }

    std::vector<world::WorldPoint> soldier_positions() {
        std::vector<world::WorldPoint> points;
        registry.each<world::Soldier>([&](core::EntityId id, world::Soldier&) {
            points.push_back(registry.get<world::Position>(id)->point);
        });
        return points;
    }

    int soldiers() {
        int count = 0;
        registry.each<world::Soldier>([&](core::EntityId, world::Soldier&) { ++count; });
        return count;
    }
};

} // namespace

TEST(RecruitSystem, RecruitingAtAFinishedBarracksSpawnsASoldierWithTwoSlots) {
    // Given
    Fixture f;
    const auto barracks = f.build(world::BuildingType::Barracks, true);

    // When
    f.bus.publish(world::SoldierRecruitRequested{barracks});

    // Then
    ASSERT_EQ(f.soldiers(), 1);
    f.registry.each<world::Soldier>([&](core::EntityId id, world::Soldier&) {
        EXPECT_TRUE(f.registry.has<world::Unit>(id));
        ASSERT_NE(f.registry.get<world::Inventory>(id), nullptr);
        EXPECT_EQ(f.registry.get<world::Inventory>(id)->capacity(), 2u);
    });
}

TEST(RecruitSystem, ABarracksUnderConstructionRecruitsNobody) {
    // Given
    Fixture f;
    const auto barracks = f.build(world::BuildingType::Barracks, false);

    // When
    f.bus.publish(world::SoldierRecruitRequested{barracks});

    // Then
    EXPECT_EQ(f.soldiers(), 0);
}

TEST(RecruitSystem, AnUnpoweredBarracksRecruitsNobody) {
    // Given
    Fixture f;
    const auto barracks = f.build(world::BuildingType::Barracks, true);
    f.registry.get<world::PowerConsumer>(barracks)->powered = false;

    // When
    f.bus.publish(world::SoldierRecruitRequested{barracks});

    // Then
    EXPECT_EQ(f.soldiers(), 0);
}

TEST(RecruitSystem, ABuildingThatIsNotABarracksRecruitsNobody) {
    // Given
    Fixture f;
    const auto steelworks = f.build(world::BuildingType::Steelworks, true);

    // When
    f.bus.publish(world::SoldierRecruitRequested{steelworks});

    // Then
    EXPECT_EQ(f.soldiers(), 0);
}

TEST(RecruitSystem, ASoldierSpawnsOnAFreeTileRightOutsideTheBarracks) {
    // Given
    Fixture f;
    const auto barracks = f.build(world::BuildingType::Barracks, true);

    // When
    f.bus.publish(world::SoldierRecruitRequested{barracks});

    // Then: on a tile centre, next to the 3x3 footprint but not inside it
    ASSERT_EQ(f.soldiers(), 1);
    const world::WorldPoint at = f.soldier_positions().front();
    const world::TileCoord tile = world::tile_containing(at);
    EXPECT_FLOAT_EQ(at.x, tile.x + 0.5f);
    EXPECT_FLOAT_EQ(at.y, tile.y + 0.5f);
    EXPECT_EQ(world::building_at(f.registry, tile), 0u);
    EXPECT_GE(tile.x, BarracksTile.x - 1);
    EXPECT_LE(tile.x, BarracksTile.x + 3);
    EXPECT_GE(tile.y, BarracksTile.y - 1);
    EXPECT_LE(tile.y, BarracksTile.y + 3);
}

TEST(RecruitSystem, SoldiersRecruitedOneAfterTheOtherTakeDifferentTiles) {
    // Given
    Fixture f;
    const auto barracks = f.build(world::BuildingType::Barracks, true);

    // When
    f.bus.publish(world::SoldierRecruitRequested{barracks});
    f.bus.publish(world::SoldierRecruitRequested{barracks});

    // Then
    const auto points = f.soldier_positions();
    ASSERT_EQ(points.size(), 2u);
    EXPECT_FALSE(world::tile_containing(points[0]) == world::tile_containing(points[1]));
}

TEST(RecruitSystem, ASoldierNeverSpawnsInsideANeighbouringBuilding) {
    // Given: another building against the barracks' left side
    Fixture f;
    f.build(world::BuildingType::Barracks, true, {BarracksTile.x - 3, BarracksTile.y});
    const auto barracks = f.build(world::BuildingType::Barracks, true);

    // When
    for (int i = 0; i < 16; ++i) {
        f.bus.publish(world::SoldierRecruitRequested{barracks});
    }

    // Then
    for (const world::WorldPoint at : f.soldier_positions()) {
        EXPECT_EQ(world::building_at(f.registry, world::tile_containing(at)), 0u);
    }
}
