#include <gtest/gtest.h>

#include <memory>

#include "event/event_bus.hpp"
#include "graphics/texture_manager.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/render/building_panel.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"
#include "support/fake_texture_loader.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};
constexpr ::world::TileCoord MineTile{1, 0}; // next to the player spawned at the origin

ui::Point centre_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

// A finished mine with 40 coal next to a new player (who carries the starting iron ore).
game::SaveGame save_with_mine() {
    game::SaveGame save{"Alpha", 0};
    save.entities.push_back({"building", "Mine",
                             {{"building.type", "Mine"},
                              {"building.x", "1"},
                              {"building.y", "0"},
                              {"inventory.0", "Coal:40"}}});
    return save;
}

class BuildingPanel : public testing::Test {
protected:
    BuildingPanel() : model(bus, input.handler, save_with_mine()), panel(model, pointer) {
        mine = model.registry().entities().front().id;
    }

    void SetUp() override {
        graphics::TextureManager::instance().reset();
        graphics::TextureManager::instance().set_loader(std::make_unique<FakeTextureLoader>());
        model.open_transfer(MineTile);
        panel.layout(canvas, Screen);
    }
    void TearDown() override { graphics::TextureManager::instance().reset(); }

    void click(ui::Point at) {
        pointer->at = {at.x, at.y};
        pointer->release = true;
        panel.update();
        pointer->release = false;
        panel.layout(canvas, Screen);
    }

    int count(core::EntityId id, ::world::Item item) const {
        return model.registry().get<::world::Inventory>(id)->count(item);
    }

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::BuildingPanel panel;
    core::EntityId mine = 0;
};

} // namespace

TEST_F(BuildingPanel, IsVisibleOnlyWhileATransferIsOpen) {
    // Given
    const bool visible_when_open = panel.visible();

    // When
    model.close_transfer();

    // Then
    EXPECT_TRUE(visible_when_open);
    EXPECT_FALSE(panel.visible());
}

TEST_F(BuildingPanel, ClickingAFilledBuildingSlotMovesItsStackToThePlayer) {
    // Given
    ASSERT_EQ(model.transfer_target(), mine);

    // When
    click(centre_of(panel.building_slot_bounds(0)));
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_EQ(count(mine, ::world::items::Coal), 0);
    EXPECT_EQ(count(model.player_id(), ::world::items::Coal), 40);
}

TEST_F(BuildingPanel, ClickingAFilledPlayerSlotMovesItsStackToTheBuilding) {
    // Given: the starting iron ore fills the player's first two slots (100 each)

    // When
    click(centre_of(panel.player_slot_bounds(0)));
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_EQ(count(mine, ::world::items::IronOre), 100);
    EXPECT_EQ(count(model.player_id(), ::world::items::IronOre), 100);
}

TEST_F(BuildingPanel, TheCloseButtonClosesTheTransfer) {
    // Given / When
    click(centre_of(panel.close_bounds()));

    // Then
    EXPECT_EQ(model.transfer_target(), 0u);
}

TEST_F(BuildingPanel, ShowsTheBuildingNameAndDetailsAboveItsInventory) {
    // Given
    FakeCanvas drawn;

    // When
    panel.draw(drawn, Screen);

    // Then: name, then state and power, top to bottom, all above the mine's first slot
    const auto y_of = [&](const std::string& text) {
        for (const DrawnText& drawn_text : drawn.texts) {
            if (drawn_text.text == text) {
                return drawn_text.position.y;
            }
        }
        ADD_FAILURE() << "not drawn: " << text;
        return 0.0f;
    };
    const float name = y_of("Mine");
    const float state = y_of("State: Working");
    const float power = y_of("Power: 100 kW");
    EXPECT_LT(name, state);
    EXPECT_LT(state, power);
    EXPECT_LT(power + 16.0f, panel.building_slot_bounds(0).y);
}

namespace {

// A finished storage (120 slots) next to the player, with its transfer open.
struct StoragePanel {
    static game::SaveGame save() {
        game::SaveGame save{"Alpha", 0};
        save.entities.push_back({"building", "Storage",
                                 {{"building.type", "Storage"},
                                  {"building.x", "1"},
                                  {"building.y", "0"}}});
        return save;
    }

    StoragePanel() : model(bus, input.handler, save()), panel(model, pointer) {
        model.open_transfer(MineTile);
        panel.layout(canvas, Screen);
    }

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::BuildingPanel panel;
};

} // namespace

TEST(BuildingPanelScroll, ABigInventoryShowsFourRowsAndTheWheelOverItScrollsIt) {
    // Given
    StoragePanel p;
    ASSERT_NE(p.model.transfer_target(), 0u);
    const ui::Rect first = p.panel.building_slot_bounds(0);
    EXPECT_FLOAT_EQ(p.panel.building_slot_bounds(40).width, 0.0f);

    // When
    p.pointer->at = {first.x + 1.0f, first.y + 1.0f};
    p.pointer->wheel = -1.0f;
    p.panel.update();
    p.panel.layout(p.canvas, Screen);

    // Then
    EXPECT_FLOAT_EQ(p.panel.building_slot_bounds(0).width, 0.0f);
    EXPECT_FLOAT_EQ(p.panel.building_slot_bounds(10).y, first.y);
}

TEST(BuildingPanelScroll, TheWheelOutsideTheBuildingInventoryDoesNotScrollIt) {
    // Given
    StoragePanel p;

    // When
    p.pointer->at = {0.0f, 0.0f};
    p.pointer->wheel = -1.0f;
    p.panel.update();
    p.panel.layout(p.canvas, Screen);

    // Then
    EXPECT_GT(p.panel.building_slot_bounds(0).width, 0.0f);
}
