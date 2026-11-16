#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "event/event_bus.hpp"
#include "graphics/texture_manager.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/core/construction/build_rules.hpp"
#include "scene/world/core/events/building_events.hpp"
#include "scene/world/render/player_hud.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"
#include "support/fake_texture_loader.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

ui::Point centre_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

class PlayerHud : public testing::Test {
protected:
    PlayerHud() : model(bus, input.handler, game::SaveGame{"Alpha", 0}), hud(model, pointer) {}

    void SetUp() override {
        graphics::TextureManager::instance().reset();
        graphics::TextureManager::instance().set_loader(std::make_unique<FakeTextureLoader>());
        model.toggle_build_bar();
        hud.layout(canvas, Screen);
    }
    void TearDown() override { graphics::TextureManager::instance().reset(); }

    // Clicks at `at` and lays the HUD out again, like a frame of update() then draw().
    void click(ui::Point at) {
        pointer->at = {at.x, at.y};
        pointer->release = true;
        hud.update();
        pointer->release = false;
        hud.layout(canvas, Screen);
    }

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::PlayerHud hud;
};

} // namespace

TEST_F(PlayerHud, IsVisibleOnlyWhileTheBuildBarIsOpenAndTheGameIsNotPaused) {
    // Given
    const bool visible_when_open = hud.visible();

    // When
    model.toggle_pause();
    const bool visible_when_paused = hud.visible();
    model.toggle_pause();
    model.toggle_build_bar();

    // Then
    EXPECT_TRUE(visible_when_open);
    EXPECT_FALSE(visible_when_paused);
    EXPECT_FALSE(hud.visible());
}

TEST_F(PlayerHud, BarTouchesTheBottomEdgeAndSpansMostOfTheWidth) {
    // Given / When
    const ui::Rect bar = hud.bar_bounds();

    // Then
    EXPECT_FLOAT_EQ(bar.y + bar.height, Screen.height);
    EXPECT_GE(bar.width, Screen.width * scene::world::PlayerHud::BarWidthCoverage - 0.5f);
}

TEST_F(PlayerHud, BarSitsCentredAtTheBottomWithOneTabPerCategory) {
    // Given / When
    const ui::Rect bar = hud.bar_bounds();

    // Then
    EXPECT_EQ(hud.tab_count(), world::construction_catalog().size());
    EXPECT_NEAR(bar.x + bar.width / 2.0f, Screen.width / 2.0f, 0.5f);
    EXPECT_GT(bar.y, Screen.height * 0.8f);
    EXPECT_LE(bar.y + bar.height, Screen.height);
    for (std::size_t i = 0; i < hud.tab_count(); ++i) {
        const ui::Rect tab = hud.tab_bounds(i);
        EXPECT_GE(tab.x, bar.x);
        EXPECT_LE(tab.x + tab.width, bar.x + bar.width);
    }
}

TEST_F(PlayerHud, ClickingATabOpensItsListAboveTheBar) {
    // Given
    const std::size_t mines = 1;

    // When
    click(centre_of(hud.tab_bounds(mines)));

    // Then
    ASSERT_EQ(hud.open_tab(), mines);
    const auto& constructions = world::construction_catalog()[mines].constructions;
    ASSERT_EQ(hud.item_count(), constructions.size());
    for (std::size_t i = 0; i < hud.item_count(); ++i) {
        EXPECT_LE(hud.item_bounds(i).y + hud.item_bounds(i).height, hud.bar_bounds().y);
    }
}

TEST_F(PlayerHud, OpenListSitsRightOnTopOfTheBar) {
    // Given / When
    click(centre_of(hud.tab_bounds(0)));

    // Then
    const ui::Rect list = hud.list_bounds();
    EXPECT_FLOAT_EQ(list.y + list.height, hud.bar_bounds().y);
}

TEST_F(PlayerHud, ClickingTheOpenTabAgainClosesItsList) {
    // Given
    click(centre_of(hud.tab_bounds(0)));

    // When
    click(centre_of(hud.tab_bounds(0)));

    // Then
    EXPECT_FALSE(hud.open_tab().has_value());
    EXPECT_EQ(hud.item_count(), 0u);
}

TEST_F(PlayerHud, ClickingAnotherTabSwitchesTheList) {
    // Given
    click(centre_of(hud.tab_bounds(0)));

    // When
    click(centre_of(hud.tab_bounds(3)));

    // Then
    EXPECT_EQ(hud.open_tab(), 3u);
    EXPECT_EQ(hud.item_count(), world::construction_catalog()[3].constructions.size());
}

TEST_F(PlayerHud, ClickingAConstructionSelectsItAndClosesTheList) {
    // Given
    click(centre_of(hud.tab_bounds(0)));

    // When
    click(centre_of(hud.item_bounds(0)));

    // Then
    ASSERT_NE(hud.selected(), nullptr);
    EXPECT_EQ(hud.selected()->name, "Headquarters");
    EXPECT_FALSE(hud.open_tab().has_value());
}

TEST_F(PlayerHud, DrawsAPlaceholderIconForEveryTabAndListedConstruction) {
    // Given
    click(centre_of(hud.tab_bounds(1)));

    // When
    hud.draw(canvas, Screen);

    // Then
    const std::size_t expected = hud.tab_count() + hud.item_count();
    ASSERT_EQ(canvas.textures.size(), expected);
    for (const auto& drawn : canvas.textures) {
        EXPECT_EQ(drawn.texture.id, FakeTextureLoader::PlaceholderId);
    }
}

TEST_F(PlayerHud, AnEntryThatCannotBeBuiltYetIsNotSelected) {
    // Given
    click(centre_of(hud.tab_bounds(0))); // Special: Headquarters, Storage

    // When
    click(centre_of(hud.item_bounds(1)));

    // Then
    EXPECT_EQ(hud.selected(), nullptr);
}

TEST_F(PlayerHud, ClickingTheWorldWithASelectionBuildsItOnTheTileUnderThePointer) {
    // Given
    std::vector<::world::BuildRequested> requests;
    const auto subscription = event::subscribe_scoped<::world::BuildRequested>(
        bus, [&](const ::world::BuildRequested& request) { requests.push_back(request); });
    click(centre_of(hud.tab_bounds(0)));
    click(centre_of(hud.item_bounds(0)));
    ASSERT_NE(hud.selected(), nullptr);
    const ui::Point target{Screen.width / 2.0f + 70.0f, Screen.height / 3.0f};
    const ::world::TileCoord expected =
        model.camera().projection(Screen.width, Screen.height).tile_at({target.x, target.y});

    // When: the request is applied on the next tick
    click(target);
    model.update(scene::world::SimulationTick);

    // Then
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests[0].type, ::world::BuildingType::Headquarters);
    EXPECT_EQ(requests[0].tile, expected);
    EXPECT_EQ(hud.selected(), nullptr);
    EXPECT_TRUE(::world::has_headquarters(model.registry()));
}

TEST_F(PlayerHud, TheHeadquartersCannotBeSelectedOnceItExists) {
    // Given
    model.request_build(::world::BuildingType::Headquarters, {2, 2});
    model.update(scene::world::SimulationTick);
    click(centre_of(hud.tab_bounds(0)));

    // When
    click(centre_of(hud.item_bounds(0)));

    // Then
    EXPECT_EQ(hud.selected(), nullptr);
}

TEST_F(PlayerHud, ClickingTheBarWithASelectionDoesNotBuild) {
    // Given
    click(centre_of(hud.tab_bounds(0)));
    click(centre_of(hud.item_bounds(0)));
    const ui::Rect bar = hud.bar_bounds();

    // When
    click({bar.x + 2.0f, bar.y + 2.0f});

    // Then
    EXPECT_FALSE(::world::has_headquarters(model.registry()));
    EXPECT_NE(hud.selected(), nullptr);
}

TEST_F(PlayerHud, ShowsNoPreviewWithoutASelection) {
    // Given
    pointer->at = {Screen.width / 2.0f + 70.0f, Screen.height / 3.0f};

    // When
    const auto preview = hud.preview();

    // Then
    EXPECT_FALSE(preview.has_value());
}

TEST_F(PlayerHud, PreviewsTheSelectionOnTheTileUnderThePointer) {
    // Given
    click(centre_of(hud.tab_bounds(0)));
    click(centre_of(hud.item_bounds(0)));
    pointer->at = {Screen.width / 2.0f + 70.0f, Screen.height / 3.0f};
    const ::world::TileCoord expected =
        model.camera().projection(Screen.width, Screen.height).tile_at({pointer->at.x, pointer->at.y});

    // When
    const auto preview = hud.preview();

    // Then
    ASSERT_TRUE(preview.has_value());
    EXPECT_EQ(preview->tile, expected);
    EXPECT_TRUE(preview->valid);
}

TEST_F(PlayerHud, ThePreviewIsInvalidOnATileThatCannotBeBuilt) {
    // Given: the headquarters is selected, then one gets built on the tile under the pointer
    click(centre_of(hud.tab_bounds(0)));
    click(centre_of(hud.item_bounds(0)));
    pointer->at = {Screen.width / 2.0f + 70.0f, Screen.height / 3.0f};
    model.request_build(::world::BuildingType::Headquarters, hud.preview()->tile);
    model.update(scene::world::SimulationTick);

    // When
    const auto preview = hud.preview();

    // Then
    ASSERT_TRUE(preview.has_value());
    EXPECT_FALSE(preview->valid);
}

TEST_F(PlayerHud, NoPreviewWhileThePointerIsOverTheBar) {
    // Given
    click(centre_of(hud.tab_bounds(0)));
    click(centre_of(hud.item_bounds(0)));
    const ui::Rect bar = hud.bar_bounds();
    pointer->at = {bar.x + 2.0f, bar.y + 2.0f};

    // When
    const auto preview = hud.preview();

    // Then
    EXPECT_FALSE(preview.has_value());
}

TEST_F(PlayerHud, ARefusedClickKeepsThePreviewAndMarksItRejected) {
    // Given: a headquarters already exists, so a second one is refused everywhere
    click(centre_of(hud.tab_bounds(0)));
    click(centre_of(hud.item_bounds(0)));
    const ui::Point target{Screen.width / 2.0f + 70.0f, Screen.height / 3.0f};
    pointer->at = {target.x, target.y};
    model.request_build(::world::BuildingType::Headquarters, {-20, -20});
    model.update(scene::world::SimulationTick);
    std::vector<::world::BuildRequested> requests;
    const auto subscription = event::subscribe_scoped<::world::BuildRequested>(
        bus, [&](const ::world::BuildRequested& request) { requests.push_back(request); });

    // When
    click(target);

    // Then
    EXPECT_TRUE(requests.empty());
    ASSERT_NE(hud.selected(), nullptr);
    ASSERT_TRUE(hud.preview().has_value());
    EXPECT_FALSE(hud.preview()->valid);
    EXPECT_TRUE(hud.preview()->rejected);
}

TEST_F(PlayerHud, TheRejectedMarkClearsOnAnotherTile) {
    // Given
    click(centre_of(hud.tab_bounds(0)));
    click(centre_of(hud.item_bounds(0)));
    model.request_build(::world::BuildingType::Headquarters, {-20, -20});
    model.update(scene::world::SimulationTick);
    click({Screen.width / 2.0f + 70.0f, Screen.height / 3.0f});

    // When
    pointer->at = {Screen.width / 2.0f - 200.0f, Screen.height / 3.0f};

    // Then
    ASSERT_TRUE(hud.preview().has_value());
    EXPECT_FALSE(hud.preview()->rejected);
}
