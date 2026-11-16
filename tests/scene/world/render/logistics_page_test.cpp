#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "core/registry.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/hauler.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/pathfinding/route_network.hpp"
#include "scene/world/core/terrain/terrain.hpp"
#include "scene/world/render/logistics_page.hpp"
#include "support/fake_canvas.hpp"

namespace {

struct Assign {
    world::RouteKey route;
    world::HaulMode mode;
    int count;
};

struct ModeChange {
    world::RouteKey route;
    world::HaulMode mode;
};

struct Page {
    core::Registry registry;
    world::Terrain terrain{7};
    world::RouteNetwork network{std::make_unique<world::LogisticRoutePlanner>()};
    std::vector<Assign> assigns;
    std::vector<ModeChange> modes;
    scene::world::LogisticsPage page{
        registry, network,
        [this](world::RouteKey route, world::HaulMode mode, int count) {
            assigns.push_back({route, mode, count});
        },
        [this](world::RouteKey route, world::HaulMode mode) { modes.push_back({route, mode}); }};
    FakeCanvas canvas;

    core::EntityId add(world::BuildingType type, world::TileCoord tile) {
        const core::EntityId id = registry.create("building");
        registry.add(id, world::Building{type, tile});
        return id;
    }

    // A storage at (0, 0) and a mine at (3, 0): one logistic route.
    world::RouteKey add_route() {
        const auto storage = add(world::BuildingType::Storage, {0, 0});
        const auto mine = add(world::BuildingType::Mine, {3, 0});
        registry.add(mine, world::Producer{});
        network.rebuild(registry, terrain);
        return {storage, mine};
    }

    void add_hauler(world::RouteKey route, world::HaulMode mode) {
        const core::EntityId id = registry.create("Worker");
        registry.add(id, world::Worker{world::WorkerState::Working, 0});
        registry.add(id, world::Hauler{route, mode});
    }

    // A storage at (0, 0) feeding six mines: one group of six routes.
    void add_six_mine_routes() {
        add(world::BuildingType::Storage, {0, 0});
        for (const world::TileCoord tile : {world::TileCoord{3, 0}, world::TileCoord{6, 0},
                                            world::TileCoord{0, 3}, world::TileCoord{3, 3},
                                            world::TileCoord{6, 3}, world::TileCoord{0, 6}}) {
            registry.add(add(world::BuildingType::Mine, tile), world::Producer{});
        }
        network.rebuild(registry, terrain);
    }

    std::vector<std::string> draw(float height = 600.0f) {
        canvas = FakeCanvas{};
        page.layout({0.0f, 0.0f, 1000.0f, height}, canvas);
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

std::vector<std::string> route_rows(const std::vector<std::string>& texts) {
    std::vector<std::string> rows;
    for (const std::string& text : texts) {
        if (text.starts_with("Storage")) rows.push_back(text);
    }
    return rows;
}

} // namespace

TEST(LogisticsPage, ListsEachLogisticRouteWithItsEndsLengthAndHaulers) {
    // Given
    Page p;
    const auto route = p.add_route();
    p.add_hauler(route, world::HaulMode::Put);

    // When
    const std::vector<std::string> texts = p.draw();

    // Then
    EXPECT_TRUE(contains(texts, "Logistic routes: 1"));
    EXPECT_TRUE(contains(texts, "Storage (0, 0) -> Mine (3, 0) - 4 tiles - 1 workers"));
    EXPECT_FALSE(p.page.take_checkbox(0).checked());
    EXPECT_TRUE(p.page.put_checkbox(0).checked());
}

TEST(LogisticsPage, SaysSoWhenThereIsNoLogisticRoute) {
    // Given
    Page p;
    p.network.rebuild(p.registry, p.terrain);

    // When
    const std::vector<std::string> texts = p.draw();

    // Then
    EXPECT_TRUE(contains(texts, "Logistic routes: 0"));
    EXPECT_TRUE(contains(texts, "No logistic routes: build a storage"));
}

TEST(LogisticsPage, ARouteWithoutHaulersStartsInTakeMode) {
    // Given
    Page p;
    p.add_route();

    // When
    p.draw();

    // Then
    EXPECT_TRUE(p.page.take_checkbox(0).checked());
    EXPECT_FALSE(p.page.put_checkbox(0).checked());
}

TEST(LogisticsPage, PlusAndMinusAskToAssignAndReleaseOneHaulerInTheRoutesMode) {
    // Given
    Page p;
    const auto route = p.add_route();
    p.draw();

    // When
    p.click(p.page.add_button(0).bounds());
    p.click(p.page.remove_button(0).bounds());

    // Then
    ASSERT_EQ(p.assigns.size(), 2u);
    EXPECT_EQ(p.assigns[0].route, route);
    EXPECT_EQ(p.assigns[0].mode, world::HaulMode::Take);
    EXPECT_EQ(p.assigns[0].count, 1);
    EXPECT_EQ(p.assigns[1].count, -1);
}

TEST(LogisticsPage, CheckingPutOnATakeRouteMakesItTakeAndPut) {
    // Given
    Page p;
    const auto route = p.add_route();
    p.draw();

    // When: check Put, then assign one
    p.click(p.page.put_checkbox(0).bounds());
    p.draw();
    p.click(p.page.add_button(0).bounds());

    // Then
    ASSERT_EQ(p.modes.size(), 1u);
    EXPECT_EQ(p.modes[0].route, route);
    EXPECT_EQ(p.modes[0].mode, world::HaulMode::Both);
    EXPECT_TRUE(p.page.take_checkbox(0).checked());
    EXPECT_TRUE(p.page.put_checkbox(0).checked());
    ASSERT_EQ(p.assigns.size(), 1u);
    EXPECT_EQ(p.assigns[0].mode, world::HaulMode::Both);
}

TEST(LogisticsPage, UncheckingTakeOnATakeAndPutRouteLeavesItPutOnly) {
    // Given
    Page p;
    const auto route = p.add_route();
    p.add_hauler(route, world::HaulMode::Both);
    p.draw();

    // When
    p.click(p.page.take_checkbox(0).bounds());
    p.draw();

    // Then
    ASSERT_EQ(p.modes.size(), 1u);
    EXPECT_EQ(p.modes[0].mode, world::HaulMode::Put);
}

TEST(LogisticsPage, TheLastCheckedBoxCannotBeUnchecked) {
    // Given: a Take-only route
    Page p;
    p.add_route();
    p.draw();

    // When
    p.click(p.page.take_checkbox(0).bounds());
    p.draw();

    // Then: a route always carries one way at least
    EXPECT_TRUE(p.modes.empty());
    EXPECT_TRUE(p.page.take_checkbox(0).checked());
}

TEST(LogisticsPage, GroupsTheRoutesUnderAHeadingPerDestinationBuildingType) {
    // Given: a storage feeding a mine, a steelworks and a second mine, built in that order
    Page p;
    p.add(world::BuildingType::Storage, {0, 0});
    for (const auto& [type, tile] : {std::pair{world::BuildingType::Mine, world::TileCoord{3, 0}},
                                     std::pair{world::BuildingType::Steelworks, world::TileCoord{0, 3}},
                                     std::pair{world::BuildingType::Mine, world::TileCoord{6, 0}}}) {
        p.registry.add(p.add(type, tile), world::Producer{});
    }
    p.network.rebuild(p.registry, p.terrain);

    // When
    const std::vector<std::string> texts = p.draw();

    // Then: each heading is followed by its own routes, the two mines together
    std::vector<std::string> lines;
    for (const std::string& text : texts) {
        if (text.starts_with("Storage") || text.starts_with("[")) {
            lines.push_back(text.substr(0, text.find(" - ")));
        }
    }
    const std::vector<std::string> expected{
        "[-] Mine (2)", "Storage (0, 0) -> Mine (3, 0)", "Storage (0, 0) -> Mine (6, 0)",
        "[-] Steelworks (1)", "Storage (0, 0) -> Steelworks (0, 3)"};
    EXPECT_EQ(lines, expected);
}

TEST(LogisticsPage, RoutesThatDoNotFitThePageAreHiddenUntilScrolledTo) {
    // Given: six routes on a page too short for all of them
    Page p;
    p.add_six_mine_routes();
    const std::vector<std::string> top = p.draw(400.0f);

    // When: scroll far past the bottom (clamped to the last lines)
    p.page.scroll(-100.0f);
    const std::vector<std::string> bottom = p.draw(400.0f);

    // Then: each view shows only part of the list, together all of it
    const auto top_rows = route_rows(top);
    const auto bottom_rows = route_rows(bottom);
    EXPECT_TRUE(contains(top, "[-] Mine (6)"));
    EXPECT_FALSE(contains(bottom, "[-] Mine (6)"));
    EXPECT_GT(top_rows.size(), 0u);
    EXPECT_LT(top_rows.size(), 6u);
    EXPECT_EQ(bottom_rows.size(), top_rows.size() + 1); // the heading's line holds a row now
    std::vector<std::string> seen = top_rows;
    for (const auto& row : bottom_rows) {
        if (std::find(seen.begin(), seen.end(), row) == seen.end()) seen.push_back(row);
    }
    EXPECT_EQ(seen.size(), 6u);
}

TEST(LogisticsPage, ScrollingUpPastTheTopStaysOnTheFirstLine) {
    // Given
    Page p;
    p.add_six_mine_routes();
    p.draw(400.0f);

    // When
    p.page.scroll(-1.0f);
    p.page.scroll(5.0f);
    const std::vector<std::string> texts = p.draw(400.0f);

    // Then
    EXPECT_EQ(p.page.first_line(), 0u);
    EXPECT_TRUE(contains(texts, "[-] Mine (6)"));
}

TEST(LogisticsPage, ClickingAGroupsHeadingHidesItsRoutesAndClickingAgainShowsThem) {
    // Given: a storage feeding a mine and a steelworks
    Page p;
    p.add(world::BuildingType::Storage, {0, 0});
    p.registry.add(p.add(world::BuildingType::Mine, {3, 0}), world::Producer{});
    p.registry.add(p.add(world::BuildingType::Steelworks, {0, 3}), world::Producer{});
    p.network.rebuild(p.registry, p.terrain);
    p.draw();

    // When
    p.click(p.page.heading_button(0).bounds());
    const std::vector<std::string> hidden = p.draw();
    p.click(p.page.heading_button(0).bounds());
    const std::vector<std::string> shown = p.draw();

    // Then: hidden, the mine route is gone and the steelworks group is untouched
    const auto mine_row = [](const std::vector<std::string>& texts) {
        for (const std::string& text : texts) {
            if (text.starts_with("Storage (0, 0) -> Mine")) return true;
        }
        return false;
    };
    EXPECT_TRUE(contains(hidden, "[+] Mine (1)"));
    EXPECT_FALSE(mine_row(hidden));
    EXPECT_TRUE(contains(hidden, "[-] Steelworks (1)"));
    EXPECT_TRUE(contains(shown, "[-] Mine (1)"));
    EXPECT_TRUE(mine_row(shown));
}
