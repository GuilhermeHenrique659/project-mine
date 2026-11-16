#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/occupancy.hpp"

namespace {

using world::TileCoord;

core::EntityId add(core::Registry& registry, TileCoord tile, world::TileSize size = {}) {
    const core::EntityId id = registry.create("building");
    registry.add(id, world::Building{world::BuildingType::Mine, tile, size});
    return id;
}

} // namespace

TEST(Occupancy, EveryTileOfAMultiTileBuildingBelongsToIt) {
    // Given
    core::Registry registry;
    world::track_buildings(registry);
    const core::EntityId big = add(registry, {3, 4}, {2, 2});

    // When
    const core::EntityId far_corner = world::building_at(registry, {4, 5});
    const core::EntityId outside = world::building_at(registry, {5, 4});

    // Then
    EXPECT_EQ(far_corner, big);
    EXPECT_EQ(outside, 0u);
}

TEST(Occupancy, ReachToABigBuildingIsMeasuredFromItsEdgeNotItsCentre) {
    // Given: a 5x5 building covering x 0..5, y 0..5 (centre 2.5, 2.5)
    core::Registry registry;
    const core::EntityId big = add(registry, {0, 0}, {5, 5});

    // When
    const float beside = world::distance_to_building(registry, big, {-1.5f, 2.5f});
    const float inside = world::distance_to_building(registry, big, {1.0f, 1.0f});
    const float off_corner = world::distance_to_building(registry, big, {8.0f, 9.0f});

    // Then
    EXPECT_FLOAT_EQ(beside, 1.5f);
    EXPECT_FLOAT_EQ(inside, 0.0f);
    EXPECT_FLOAT_EQ(off_corner, 5.0f);
}

TEST(Occupancy, ReachToAnEntityWithoutABuildingIsMeasuredFromItsPosition) {
    // Given
    core::Registry registry;
    const core::EntityId crate = registry.create("crate");
    registry.add(crate, world::Position{{1.0f, 1.0f}});

    // When
    const float distance = world::distance_to_building(registry, crate, {4.0f, 5.0f});

    // Then
    EXPECT_FLOAT_EQ(distance, 5.0f);
}

TEST(Occupancy, TheIndexFollowsABuildingThatMovesOrIsDestroyed) {
    // Given
    core::Registry registry;
    world::track_buildings(registry);
    const core::EntityId id = add(registry, {1, 1});
    const core::EntityId other = add(registry, {6, 6});

    // When
    registry.add(id, world::Building{world::BuildingType::Mine, {2, 1}, {}});
    registry.destroy(other);

    // Then
    EXPECT_EQ(world::building_at(registry, {1, 1}), 0u);
    EXPECT_EQ(world::building_at(registry, {2, 1}), id);
    EXPECT_EQ(world::building_at(registry, {6, 6}), 0u);
}

TEST(Occupancy, TrackingARegistryIndexesTheBuildingsItAlreadyHas) {
    // Given
    core::Registry registry;
    const core::EntityId id = add(registry, {-2, 3}, {1, 3});

    // When
    world::track_buildings(registry);

    // Then
    EXPECT_EQ(world::building_at(registry, {-2, 5}), id);
    EXPECT_NE(registry.find_context<world::Occupancy>(), nullptr);
}

TEST(Occupancy, AnUntrackedRegistryIsSearchedDirectly) {
    // Given
    core::Registry registry;
    const core::EntityId id = add(registry, {0, 0}, {2, 1});

    // When
    const core::EntityId found = world::building_at(registry, {1, 0});

    // Then
    EXPECT_EQ(found, id);
}

TEST(Occupancy, AMultiTileBuildingStandsAtTheCentreOfItsFootprint) {
    // Given
    core::Registry registry;
    world::Construction factory{"Factory", "icon", world::BuildingType::Mine};
    factory.size = {2, 3};
    const core::EntityId id = world::spawn_building(registry, "Factory");

    // When
    world::make_building(registry, id, factory, {4, -1});

    // Then
    const auto* position = registry.get<world::Position>(id);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->point.x, 5.0f);
    EXPECT_FLOAT_EQ(position->point.y, 0.5f);
    EXPECT_EQ(world::building_at(registry, {5, 1}), id);
}
