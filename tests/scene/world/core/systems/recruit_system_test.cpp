#include <gtest/gtest.h>

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
#include "scene/world/core/systems/recruit_system.hpp"

namespace {

constexpr world::TileCoord BarracksTile{4, 2};

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::EntityFactory factory = world::make_entity_factory();
    world::RecruitSystem system{registry, bus, factory};

    core::EntityId build(world::BuildingType type, bool finished) {
        const core::EntityId id = *factory.spawn(registry, world::BuildingArchetype, "Building");
        world::make_building(registry, id, *world::find_construction(type), BarracksTile);
        if (finished) {
            registry.remove<world::ConstructionSite>(id);
        }
        return id;
    }

    int soldiers() {
        int count = 0;
        registry.each<world::Soldier>([&](core::EntityId, world::Soldier&) { ++count; });
        return count;
    }
};

} // namespace

TEST(RecruitSystem, RecruitingAtAFinishedBarracksSpawnsASoldierWithTwoSlotsOnIt) {
    // Given
    Fixture f;
    const auto barracks = f.build(world::BuildingType::Barracks, true);
    const world::WorldPoint at = f.registry.get<world::Position>(barracks)->point;

    // When
    f.bus.publish(world::SoldierRecruitRequested{barracks});

    // Then
    ASSERT_EQ(f.soldiers(), 1);
    f.registry.each<world::Soldier>([&](core::EntityId id, world::Soldier&) {
        EXPECT_TRUE(f.registry.has<world::Unit>(id));
        ASSERT_NE(f.registry.get<world::Inventory>(id), nullptr);
        EXPECT_EQ(f.registry.get<world::Inventory>(id)->capacity(), 2u);
        EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.x, at.x);
        EXPECT_FLOAT_EQ(f.registry.get<world::Position>(id)->point.y, at.y);
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
