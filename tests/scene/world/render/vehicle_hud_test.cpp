#include <gtest/gtest.h>

#include <memory>

#include "event/event_bus.hpp"
#include "game/save_game.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/render/vehicle_hud.hpp"
#include "support/always_hit.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

ui::Point centre_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

// A new game with one soldier beside the starting tank.
game::SaveGame save_with_soldier() {
    game::SaveGame save{"Alpha", 0};
    save.entities.push_back({"soldier", "Soldier", {{"position.x", "1.5"}, {"position.y", "1.5"}}});
    return save;
}

class VehicleHud : public testing::Test {
protected:
    VehicleHud() : model(bus, input.handler, save_with_soldier(), AlwaysHit), hud(model, pointer) {
        for (const core::Entity& entity : model.registry().entities()) {
            if (model.registry().has<::world::Soldier>(entity.id)) {
                soldier = entity.id;
            }
        }
    }

    // Box-selects what is drawn on `point` (in the model's camera, on Screen).
    void select_at(::world::WorldPoint point) {
        const ::world::IsoProjection projection = model.camera().projection(Screen.width, Screen.height);
        const ::world::ScreenPoint at = projection.world_to_screen(point);
        model.select_units(projection, {at.x - 5.0f, at.y - 5.0f}, {at.x + 5.0f, at.y + 5.0f});
    }

    // The soldier gets into the tank, then the tank is selected.
    void load_and_select_the_tank() {
        select_at({1.5f, 1.5f});
        model.order_units(::world::tile_containing(scene::world::StartingTankPosition));
        model.update(scene::world::SimulationTick);
        model.update(scene::world::SimulationTick);
        select_at(scene::world::StartingTankPosition);
        hud.layout(canvas, Screen);
    }

    void click(ui::Point at) {
        pointer->at = {at.x, at.y};
        pointer->release = true;
        hud.update();
        pointer->release = false;
    }

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::VehicleHud hud;
    core::EntityId soldier = 0;
};

} // namespace

TEST_F(VehicleHud, IsHiddenWhileNoCommandedVehicleCarriesUnits) {
    // Given: the starting tank is selected but empty
    select_at(scene::world::StartingTankPosition);

    // When
    const bool shown = hud.visible();

    // Then
    EXPECT_FALSE(shown);
}

TEST_F(VehicleHud, ShowsTheUnloadButtonInTheBottomLeftCornerWhenTheTankHasUnits) {
    // Given
    load_and_select_the_tank();

    // When
    const bool shown = hud.visible();

    // Then
    ASSERT_TRUE(shown);
    const ui::Rect button = hud.unload_bounds();
    EXPECT_GT(button.width, 0.0f);
    EXPECT_LT(button.x, Screen.width / 4.0f);
    EXPECT_GT(button.y, Screen.height * 0.75f);
    EXPECT_LE(button.y + button.height, Screen.height);
    EXPECT_TRUE(hud.captures_pointer(centre_of(button)));
}

TEST_F(VehicleHud, IsHiddenWhilePaused) {
    // Given
    load_and_select_the_tank();

    // When
    model.toggle_pause();

    // Then
    EXPECT_FALSE(hud.visible());
}

TEST_F(VehicleHud, ClickingUnloadLetsTheUnitsOut) {
    // Given
    load_and_select_the_tank();
    ASSERT_TRUE(model.registry().has<::world::Passenger>(soldier));

    // When
    click(centre_of(hud.unload_bounds()));
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_FALSE(model.registry().has<::world::Passenger>(soldier));
    EXPECT_FALSE(hud.visible());
}

TEST_F(VehicleHud, ClickingElsewhereUnloadsNothing) {
    // Given
    load_and_select_the_tank();

    // When
    click({Screen.width / 2.0f, Screen.height / 2.0f});
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_TRUE(model.registry().has<::world::Passenger>(soldier));
}
