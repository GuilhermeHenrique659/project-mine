#include <gtest/gtest.h>

#include <cmath>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/systems/character_move_system.hpp"
#include "scene/world/core/terrain/terrain.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/unit.hpp"
#include "scene/world/core/systems/unit_traffic_system.hpp"

namespace {

constexpr float Frame = 0.05f;

struct Fixture {
    core::Registry registry;
    world::UnitTrafficSystem system{registry};

    core::EntityId add_unit(world::WorldPoint at, world::WorldPoint direction) {
        const core::EntityId id = registry.create("unit");
        registry.add(id, world::Unit{});
        registry.add(id, world::Position{at});
        registry.add(id, world::Mover{4.0f, direction});
        return id;
    }

    const world::WorldPoint& direction(core::EntityId id) {
        return registry.get<world::Mover>(id)->direction;
    }
};

} // namespace

TEST(UnitTrafficSystem, UnitsWalkingSideBySideArePushedApartSideways) {
    // Given: both walking +x, a little apart on y
    Fixture f;
    const auto upper = f.add_unit({1.0f, 0.5f}, {1.0f, 0.0f});
    const auto lower = f.add_unit({1.0f, 0.6f}, {1.0f, 0.0f});

    // When
    f.system.update(Frame);

    // Then: apart on y, still fully forward on x
    EXPECT_LT(f.direction(upper).y, 0.0f);
    EXPECT_GT(f.direction(lower).y, 0.0f);
    EXPECT_FLOAT_EQ(f.direction(upper).x, 1.0f);
    EXPECT_FLOAT_EQ(f.direction(lower).x, 1.0f);
}

TEST(UnitTrafficSystem, AUnitRightBehindAnotherStepsAsideInsteadOfSlowingDown) {
    // Given: one right behind the other, same way
    Fixture f;
    f.add_unit({1.0f, 0.5f}, {1.0f, 0.0f});
    const auto behind = f.add_unit({0.8f, 0.5f}, {1.0f, 0.0f});

    // When
    f.system.update(Frame);

    // Then
    EXPECT_FLOAT_EQ(f.direction(behind).x, 1.0f);
    EXPECT_NE(f.direction(behind).y, 0.0f);
}

TEST(UnitTrafficSystem, TheSidewaysPushIsCappedSoAUnitAlwaysAdvances) {
    // Given: squeezed between two units on the same side
    Fixture f;
    const auto squeezed = f.add_unit({1.0f, 0.5f}, {1.0f, 0.0f});
    f.add_unit({1.0f, 0.52f}, {1.0f, 0.0f});
    f.add_unit({1.0f, 0.53f}, {1.0f, 0.0f});

    // When
    f.system.update(Frame);

    // Then
    EXPECT_LE(std::abs(f.direction(squeezed).y), world::MaxSidewaysPush + 1e-5f);
}

TEST(UnitTrafficSystem, StandingUnitsAreNeitherPushedNorPushOthers) {
    // Given: workers waiting on the same spot, one walking through them
    Fixture f;
    const auto waiting = f.add_unit({1.0f, 0.5f}, {});
    f.add_unit({1.0f, 0.5f}, {});
    const auto walking = f.add_unit({1.0f, 0.55f}, {1.0f, 0.0f});

    // When
    f.system.update(Frame);

    // Then
    EXPECT_FLOAT_EQ(f.direction(waiting).x, 0.0f);
    EXPECT_FLOAT_EQ(f.direction(waiting).y, 0.0f);
    EXPECT_FLOAT_EQ(f.direction(walking).y, 0.0f);
}

TEST(UnitTrafficSystem, UnitsFarApartAreNotPushed) {
    // Given
    Fixture f;
    const auto a = f.add_unit({1.0f, 0.5f}, {1.0f, 0.0f});
    f.add_unit({1.0f, 1.5f}, {1.0f, 0.0f});

    // When
    f.system.update(Frame);

    // Then
    EXPECT_FLOAT_EQ(f.direction(a).y, 0.0f);
}

namespace {

// A unit at (0.5, 0.5) pressing +x into a building on (1, 0)..(3, 0), with real collision.
struct StuckFixture {
    StuckFixture() {
        for (int x = 1; x <= 3; ++x) {
            const core::EntityId id = f.registry.create("Wall");
            f.registry.add(id, world::Building{world::BuildingType::Mine, {x, 0}});
        }
        unit = f.add_unit({0.5f, 0.5f}, {1.0f, 0.0f});
        f.registry.add(unit, world::Character{}); // units are characters: they move
    }

    // Keeps pressing +x for `seconds`.
    void press(float seconds) {
        for (float t = 0.0f; t < seconds; t += Frame) {
            f.registry.get<world::Mover>(unit)->direction = {1.0f, 0.0f};
            f.system.update(Frame);
            move.update(Frame);
        }
    }

    float x() { return f.registry.get<world::Position>(unit)->point.x; }
    bool ghost() {
        const auto* traffic = f.registry.get<world::UnitTraffic>(unit);
        return traffic != nullptr && traffic->ghost;
    }

    Fixture f;
    event::EventBus bus;
    world::Terrain terrain{777};
    world::CharacterMoveSystem move{f.registry, bus, &terrain};
    core::EntityId unit = 0;
};

} // namespace

TEST(UnitTrafficSystem, AUnitStuckForAWhileWalksThroughWhatBlocksIt) {
    // Given
    StuckFixture s;
    s.press(world::StuckTime - 0.5f);
    const float before = s.x();

    // When
    s.press(1.5f);

    // Then
    EXPECT_LT(before, 1.0f);
    EXPECT_GT(s.x(), 1.0f);
}

TEST(UnitTrafficSystem, AUnitCollidesAgainOnceItHasMovedATile) {
    // Given: it went through
    StuckFixture s;
    s.press(world::StuckTime + 0.2f);
    const bool ghost_when_stuck = s.ghost();

    // When: well over a tile further
    s.press(1.0f);

    // Then
    EXPECT_TRUE(ghost_when_stuck);
    EXPECT_FALSE(s.ghost());
}

TEST(UnitTrafficSystem, AStandingUnitIsNeverTakenForStuck) {
    // Given: a worker waiting at the headquarters
    Fixture f;
    const auto waiting = f.add_unit({0.5f, 0.5f}, {});

    // When
    for (float t = 0.0f; t < 3.0f * world::StuckTime; t += Frame) f.system.update(Frame);

    // Then
    const auto* traffic = f.registry.get<world::UnitTraffic>(waiting);
    EXPECT_TRUE(traffic == nullptr || !traffic->ghost);
}
