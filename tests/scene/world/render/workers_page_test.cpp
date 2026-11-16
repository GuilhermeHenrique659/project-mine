#include <gtest/gtest.h>

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

    core::EntityId add_mine(world::TileCoord tile) {
        const core::EntityId id = registry.create("Mine");
        registry.add(id, world::Building{world::BuildingType::Mine, tile});
        registry.add(id, world::Staffing{10});
        return id;
    }

    void add_worker(core::EntityId workplace) {
        const core::EntityId id = registry.create("Worker");
        registry.add(id, workplace == 0 ? world::Worker{}
                                        : world::Worker{world::WorkerState::Working, workplace});
    }

    std::vector<std::string> draw() {
        page.layout({0.0f, 0.0f, 800.0f, 600.0f}, canvas);
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
    EXPECT_TRUE(contains(texts, "Mine (4, 2) - 1/10 workers"));
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
