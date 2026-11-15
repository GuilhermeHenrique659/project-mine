#include <gtest/gtest.h>

#include <memory>

#include "event/event_bus.hpp"
#include "scene/world/render/player_menu.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

// A save whose player carries 50 iron ore in its first slot.
game::SaveGame save_with_iron() {
    game::SaveGame save{"Alpha", 0};
    save.entities.push_back(
        {"character", "Player", {{"player", "1"}, {"inventory.0", "IronOre:50"}}});
    return save;
}

struct Menu {
    explicit Menu(game::SaveGame save = {"Alpha", 0})
        : model(bus, input.handler, std::move(save)), menu(model, pointer) {}

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::PlayerMenu menu;
};

} // namespace

TEST(PlayerMenu, IsVisibleOnlyWhileOpenAndTheGameIsNotPaused) {
    // Given
    Menu m;
    const bool visible_when_closed = m.menu.visible();

    // When
    m.model.toggle_player_menu();
    const bool visible_when_open = m.menu.visible();
    m.model.toggle_pause();

    // Then
    EXPECT_FALSE(visible_when_closed);
    EXPECT_TRUE(visible_when_open);
    EXPECT_FALSE(m.menu.visible());
}

TEST(PlayerMenu, OpensOnTheInventoryTabWithStatisticsAndLogisticsDisabled) {
    // Given
    Menu m;

    // When
    const ui::TabBar& tabs = m.menu.tab_bar();

    // Then
    ASSERT_EQ(tabs.tabs().size(), 3u);
    EXPECT_EQ(tabs.tabs()[0].title, "Inventory");
    EXPECT_EQ(tabs.tabs()[1].title, "Statistics");
    EXPECT_EQ(tabs.tabs()[2].title, "Logistics");
    EXPECT_EQ(tabs.active(), 0u);
    EXPECT_TRUE(tabs.tabs()[0].enabled);
    EXPECT_FALSE(tabs.tabs()[1].enabled);
    EXPECT_FALSE(tabs.tabs()[2].enabled);
}

TEST(PlayerMenu, CoversAlmostTheWholeScreenCentred) {
    // Given
    Menu m;

    // When
    m.menu.layout(m.canvas, Screen);

    // Then
    const ui::Rect panel = m.menu.panel_bounds();
    EXPECT_GE(panel.width, Screen.width * 0.85f);
    EXPECT_GE(panel.height, Screen.height * 0.85f);
    EXPECT_LT(panel.width, Screen.width);
    EXPECT_LT(panel.height, Screen.height);
    EXPECT_FLOAT_EQ(panel.x, Screen.width - panel.x - panel.width);
    EXPECT_FLOAT_EQ(panel.y, Screen.height - panel.y - panel.height);
}

TEST(PlayerMenu, TheInventoryTabShowsThePlayersItems) {
    // Given
    Menu m(save_with_iron());
    m.model.toggle_player_menu();

    // When
    m.menu.draw(m.canvas, Screen);

    // Then
    bool counted = false;
    for (const DrawnText& text : m.canvas.texts) {
        if (text.text == "50") counted = true;
    }
    EXPECT_TRUE(counted);
}

TEST(PlayerMenu, ClickingADisabledTabKeepsTheInventoryOpen) {
    // Given
    Menu m;
    m.model.toggle_player_menu();
    m.menu.layout(m.canvas, Screen);
    const ui::Rect statistics = m.menu.tab_bar().tab_bounds()[1];
    m.pointer->at = {statistics.x + statistics.width / 2.0f, statistics.y + statistics.height / 2.0f};
    m.pointer->release = true;

    // When
    m.menu.update();

    // Then
    EXPECT_EQ(m.menu.tab_bar().active(), 0u);
}

TEST(PlayerMenu, ClickingTheCloseButtonClosesTheMenu) {
    // Given
    Menu m;
    m.model.toggle_player_menu();
    m.menu.layout(m.canvas, Screen);
    const ui::Rect close = m.menu.close_button().bounds();
    m.pointer->at = {close.x + close.width / 2.0f, close.y + close.height / 2.0f};
    m.pointer->release = true;

    // When
    m.menu.update();

    // Then
    EXPECT_FALSE(m.model.is_player_menu_open());
    EXPECT_FALSE(m.model.is_paused());
}

TEST(PlayerMenu, TheCloseButtonIsAnXInTheTopRightCornerOfThePanel) {
    // Given
    Menu m;

    // When
    m.menu.layout(m.canvas, Screen);

    // Then
    const ui::Rect panel = m.menu.panel_bounds();
    const ui::Rect close = m.menu.close_button().bounds();
    EXPECT_EQ(m.menu.close_button().text(), "X");
    EXPECT_GT(close.x, panel.x + panel.width / 2.0f);
    EXPECT_LE(close.x + close.width, panel.x + panel.width);
    EXPECT_GE(close.y, panel.y);
    EXPECT_LT(close.y, panel.y + panel.height / 4.0f);
}
