#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/systems/power_system.hpp"
#include "scene/world/render/energy_page.hpp"
#include "support/fake_canvas.hpp"

namespace {

struct Page {
    core::Registry registry;
    world::PowerSystem system{registry};
    scene::world::EnergyPage page{registry};
    FakeCanvas canvas;

    void build(world::BuildingType type, std::string name, world::TileCoord tile) {
        const core::EntityId id = registry.create(std::move(name));
        world::make_building(registry, id, *world::find_construction(type), tile);
        registry.remove<world::ConstructionSite>(id);
    }

    // The drawn texts, top to bottom.
    std::vector<std::string> lines() {
        system.update(1.0f);
        page.layout({0.0f, 0.0f, 800.0f, 600.0f}, canvas);
        page.draw(canvas);
        std::vector<std::string> texts;
        float last_y = -1.0f;
        for (const DrawnText& text : canvas.texts) {
            EXPECT_GT(text.position.y, last_y) << text.text;
            last_y = text.position.y;
            texts.push_back(text.text);
        }
        return texts;
    }
};

} // namespace

TEST(EnergyPage, ShowsTheSummaryThenTheGeneratorsThenTheConsumers) {
    // Given
    Page p;
    p.build(world::BuildingType::Headquarters, "Headquarters", {0, 0});
    p.build(world::BuildingType::Mine, "Mine", {1, 1});
    p.build(world::BuildingType::Mine, "Mine", {2, 2});

    // When
    const std::vector<std::string> lines = p.lines();

    // Then
    EXPECT_EQ(lines, (std::vector<std::string>{
                         "Produced: 1000 kW",
                         "Consumed: 200 kW",
                         "Generators (1)",
                         "Headquarters #1 - 1000 kW",
                         "Consumers (2)",
                         "Mine #2 - 100 kW",
                         "Mine #3 - 100 kW",
                     }));
}

TEST(EnergyPage, WarnsWhenOverloadedAndMarksTheConsumersWithoutPower) {
    // Given: eleven mines need 1100 kW of the 1000 kW
    Page p;
    p.build(world::BuildingType::Headquarters, "Headquarters", {0, 0});
    for (int i = 1; i <= 11; ++i) {
        p.build(world::BuildingType::Mine, "Mine", {i, i});
    }

    // When
    const std::vector<std::string> lines = p.lines();

    // Then
    ASSERT_GE(lines.size(), 5u);
    EXPECT_EQ(lines[1], "Consumed: 1100 kW");
    EXPECT_EQ(lines[2], "Overloaded: consumers are idle until production covers demand");
    EXPECT_EQ(lines.back(), "Mine #12 - 100 kW (no power)");
}

TEST(EnergyPage, AnEmptyListShowsNone) {
    // Given: no building at all
    Page p;

    // When
    const std::vector<std::string> lines = p.lines();

    // Then
    EXPECT_EQ(lines, (std::vector<std::string>{"Produced: 0 kW", "Consumed: 0 kW",
                                               "Generators (0)", "None", "Consumers (0)",
                                               "None"}));
}
