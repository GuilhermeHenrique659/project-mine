#include <gtest/gtest.h>

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

    std::vector<std::string> draw() {
        canvas = FakeCanvas{};
        page.layout({0.0f, 0.0f, 1000.0f, 600.0f}, canvas);
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
    EXPECT_TRUE(contains(texts, "Put"));
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
    const std::vector<std::string> texts = p.draw();

    // Then
    EXPECT_TRUE(contains(texts, "Take"));
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

TEST(LogisticsPage, TheModeButtonSwitchesTheRouteBetweenTakeAndPut) {
    // Given
    Page p;
    const auto route = p.add_route();
    p.draw();

    // When: switch, then assign one
    p.click(p.page.mode_button(0).bounds());
    const std::vector<std::string> texts = p.draw();
    p.click(p.page.add_button(0).bounds());

    // Then
    ASSERT_EQ(p.modes.size(), 1u);
    EXPECT_EQ(p.modes[0].route, route);
    EXPECT_EQ(p.modes[0].mode, world::HaulMode::Put);
    EXPECT_TRUE(contains(texts, "Put"));
    ASSERT_EQ(p.assigns.size(), 1u);
    EXPECT_EQ(p.assigns[0].mode, world::HaulMode::Put);
}
