#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/render/workers_page.hpp"
#include "support/fake_canvas.hpp"

namespace {

struct Page {
    core::Registry registry;
    std::vector<std::pair<core::EntityId, int>> requests;
    scene::world::WorkersPage page{registry, [this](core::EntityId id, int count) {
                                       requests.emplace_back(id, count);
                                   }};
    FakeCanvas canvas;

    core::EntityId add_workplace(world::BuildingType type, const char* name,
                                 world::TileCoord tile) {
        const core::EntityId id = registry.create(name);
        registry.add(id, world::Building{type, tile});
        registry.add(id, world::Staffing{10});
        return id;
    }

    core::EntityId add_mine(world::TileCoord tile) {
        return add_workplace(world::BuildingType::Mine, "Mine", tile);
    }

    void add_worker(core::EntityId workplace) {
        const core::EntityId id = registry.create("Worker");
        registry.add(id, workplace == 0 ? world::Worker{}
                                        : world::Worker{world::WorkerState::Working, workplace});
    }

    std::vector<std::string> draw(float height = 600.0f) {
        canvas = FakeCanvas{};
        page.layout({0.0f, 0.0f, 800.0f, height}, canvas);
        page.draw(canvas);
        std::vector<std::string> texts;
        for (const DrawnText& text : canvas.texts) {
            texts.push_back(text.text);
        }
        return texts;
    }

    void click(const ui::Rect& at) {
        page.handle_pointer({at.x + at.width / 2.0f, at.y + at.height / 2.0f}, true);
    }
};

bool contains(const std::vector<std::string>& texts, const std::string& wanted) {
    for (const std::string& text : texts) {
        if (text == wanted) return true;
    }
    return false;
}

} // namespace

TEST(WorkersPage, ShowsTheIdleWorkersAndEachWorkplaceWithItsAssignedWorkers) {
    // Given
    Page p;
    const auto mine = p.add_mine({4, 2});
    p.add_worker(0);
    p.add_worker(0);
    p.add_worker(mine);

    // When
    const std::vector<std::string> texts = p.draw();

    // Then
    EXPECT_TRUE(contains(texts, "Idle workers: 2"));
    EXPECT_TRUE(contains(texts, "Mine #1 (4, 2) - 1/10 workers"));
}

TEST(WorkersPage, ClickingPlusAndMinusAsksToAssignAndReleaseOneWorker) {
    // Given
    Page p;
    const auto mine = p.add_mine({4, 2});
    p.add_worker(0);
    p.draw();

    // When
    p.click(p.page.add_button(0).bounds());
    p.click(p.page.remove_button(0).bounds());

    // Then
    ASSERT_EQ(p.requests.size(), 2u);
    EXPECT_EQ(p.requests[0], std::make_pair(mine, 1));
    EXPECT_EQ(p.requests[1], std::make_pair(mine, -1));
}

TEST(WorkersPage, SaysSoWhenNoBuildingTakesWorkers) {
    // Given
    Page p;

    // When
    const std::vector<std::string> texts = p.draw();

    // Then
    EXPECT_TRUE(contains(texts, "No building needs workers"));
}

TEST(WorkersPage, GroupsTheWorkplacesUnderAHeadingPerBuildingType) {
    // Given: a mine, a steelworks and a second mine, built in that order
    Page p;
    p.add_mine({3, 0});
    p.add_workplace(world::BuildingType::Steelworks, "Steelworks", {0, 3});
    p.add_mine({6, 0});

    // When
    const std::vector<std::string> texts = p.draw();

    // Then: each heading is followed by its own workplaces, the two mines together
    std::vector<std::string> lines;
    for (const std::string& text : texts) {
        if (text.starts_with("[") || text.ends_with("/10 workers")) lines.push_back(text);
    }
    const std::vector<std::string> expected{
        "[-] Mine (2)", "Mine #1 (3, 0) - 0/10 workers", "Mine #3 (6, 0) - 0/10 workers",
        "[-] Steelworks (1)", "Steelworks #2 (0, 3) - 0/10 workers"};
    EXPECT_EQ(lines, expected);
}

TEST(WorkersPage, ClickingAGroupsHeadingHidesItsWorkplacesAndClickingAgainShowsThem) {
    // Given: a mine and a steelworks
    Page p;
    p.add_mine({3, 0});
    p.add_workplace(world::BuildingType::Steelworks, "Steelworks", {0, 3});
    p.draw();

    // When
    p.click(p.page.heading_button(0).bounds());
    const std::vector<std::string> hidden = p.draw();
    p.click(p.page.heading_button(0).bounds());
    const std::vector<std::string> shown = p.draw();

    // Then: hidden, the mine row is gone and the steelworks group is untouched
    EXPECT_TRUE(contains(hidden, "[+] Mine (1)"));
    EXPECT_FALSE(contains(hidden, "Mine #1 (3, 0) - 0/10 workers"));
    EXPECT_TRUE(contains(hidden, "Steelworks #2 (0, 3) - 0/10 workers"));
    EXPECT_TRUE(contains(shown, "[-] Mine (1)"));
    EXPECT_TRUE(contains(shown, "Mine #1 (3, 0) - 0/10 workers"));
}

TEST(WorkersPage, KeepsItsButtonsBetweenLayoutsWhileNothingChanges) {
    // Given
    Page p;
    p.add_mine({4, 2});
    p.draw();
    const ui::Button* before = &p.page.add_button(0);

    // When
    p.draw();

    // Then: the same button, so its hover and focus survive the frame
    EXPECT_EQ(&p.page.add_button(0), before);
}
