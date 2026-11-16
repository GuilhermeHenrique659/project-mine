#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/power.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/render/building_details.hpp"

using Lines = std::vector<std::string>;

TEST(BuildingDetails, AWorkingMineShowsItsStateAndPowerConsumption) {
    // Given
    core::Registry registry;
    const core::EntityId mine = registry.create("Mine");
    world::make_building(registry, mine, *world::find_construction(world::BuildingType::Mine),
                         {1, 1});

    // When
    const Lines lines = scene::world::building_details(registry, mine);

    // Then
    EXPECT_EQ(lines, (Lines{"State: Working", "Power: 100 kW"}));
}

TEST(BuildingDetails, AnIdleBuildingShowsWhyItIsIdle) {
    // Given
    core::Registry registry;
    const core::EntityId id = registry.create("Mine");
    const std::vector<std::pair<world::IdleReason, std::string>> cases{
        {world::IdleReason::NoPower, "State: Idle (no power)"},
        {world::IdleReason::NoResource, "State: Idle (no ore)"},
        {world::IdleReason::MissingInput, "State: Idle (missing input)"},
        {world::IdleReason::OutputFull, "State: Idle (output full)"},
    };

    for (const auto& [reason, expected] : cases) {
        // When
        registry.add(id, world::BuildingStatus{world::BuildingState::Idle, reason});
        const Lines lines = scene::world::building_details(registry, id);

        // Then
        EXPECT_EQ(lines, (Lines{expected}));
    }
}

TEST(BuildingDetails, AGeneratorShowsThePowerItGenerates) {
    // Given
    core::Registry registry;
    const core::EntityId hq = registry.create("Headquarters");
    world::make_building(registry, hq,
                         *world::find_construction(world::BuildingType::Headquarters), {0, 0});

    // When
    const Lines lines = scene::world::building_details(registry, hq);

    // Then
    EXPECT_EQ(lines, (Lines{"Generates: 1000 kW"}));
}

TEST(BuildingDetails, ABuildingWithoutStatusOrPowerHasNoDetails) {
    // Given
    core::Registry registry;
    const core::EntityId id = registry.create("Storage");
    registry.add(id, world::Building{});

    // When
    const Lines lines = scene::world::building_details(registry, id);

    // Then
    EXPECT_TRUE(lines.empty());
}
