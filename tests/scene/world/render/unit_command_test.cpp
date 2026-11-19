#include <gtest/gtest.h>

#include <memory>

#include "event/event_bus.hpp"
#include "scene/world/core/components/combat.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/render/unit_command.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

// A save holding one soldier on (2.5, 2.5) (the player spawns at the origin).
game::SaveGame save_with_soldier() {
    game::SaveGame save{"Alpha", 0};
    save.entities.push_back(
        {"soldier", "Soldier", {{"position.x", "2.5"}, {"position.y", "2.5"}}});
    return save;
}

class UnitCommand : public testing::Test {
protected:
    UnitCommand() : model(bus, input.handler, save_with_soldier()), command(model, pointer) {
        for (const core::Entity& entity : model.registry().entities()) {
            if (entity.id != model.player_id()) {
                soldier = entity.id;
            }
        }
        command.draw(canvas, Screen);
        soldier_on_screen = projection().world_to_screen({2.5f, 2.5f});
    }

    ::world::IsoProjection projection() const {
        return model.camera().projection(Screen.width, Screen.height);
    }

    // A frame with the left button held at `at`.
    void hold(input::PointerPosition at) {
        pointer->at = at;
        pointer->held = true;
        pointer->release = false;
        command.update();
    }
    // The frame the left button is released at `at`.
    void release(input::PointerPosition at) {
        pointer->at = at;
        pointer->held = false;
        pointer->release = true;
        command.update();
        pointer->release = false;
    }
    // A right click at `at`: held for a frame, then released.
    void right_click(input::PointerPosition at) {
        pointer->at = at;
        pointer->right_held = true;
        command.update();
        pointer->right_held = false;
        pointer->right_release = true;
        command.update();
        pointer->right_release = false;
    }
    void drag(input::PointerPosition from, input::PointerPosition to) {
        hold(from);
        hold(to);
        release(to);
    }
    void select_soldier() {
        drag({soldier_on_screen.x - 30, soldier_on_screen.y - 30},
             {soldier_on_screen.x + 30, soldier_on_screen.y + 30});
    }

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::UnitCommand command;
    core::EntityId soldier = 0;
    ::world::ScreenPoint soldier_on_screen;
};

} // namespace

TEST_F(UnitCommand, DraggingABoxOverASoldierSelectsIt) {
    // Given / When
    select_soldier();

    // Then
    EXPECT_EQ(model.selected_units(), std::vector<core::EntityId>{soldier});
}

TEST_F(UnitCommand, DraggingABoxOverNothingClearsTheSelection) {
    // Given
    select_soldier();

    // When
    drag({10, 10}, {60, 60});

    // Then
    EXPECT_TRUE(model.selected_units().empty());
}

TEST_F(UnitCommand, ARightClickSendsTheSelectionToTheTileUnderThePointer) {
    // Given
    select_soldier();
    const ::world::ScreenPoint goal = projection().tile_center({6, 2});

    // When
    right_click({goal.x, goal.y});
    model.update(scene::world::SimulationTick);

    // Then
    const auto* path = model.registry().get<::world::PathFollower>(soldier);
    ASSERT_NE(path, nullptr);
    EXPECT_EQ(path->tiles.back(), (::world::TileCoord{6, 2}));
}

TEST_F(UnitCommand, ReleasingADragDoesNotOrderAMove) {
    // Given
    select_soldier();

    // When
    select_soldier();
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_FALSE(model.registry().has<::world::PathFollower>(soldier));
}

TEST_F(UnitCommand, ALeftClickWithoutDraggingClearsTheSelection) {
    // Given
    select_soldier();

    // When: a plain click (SiteWork still builds or opens a transfer with it)
    hold({10, 10});
    release({10, 10});

    // Then
    EXPECT_TRUE(model.selected_units().empty());
}

TEST_F(UnitCommand, ADragTakesThePointerFromTheWorldBelow) {
    // Given
    hold({100, 100});
    EXPECT_FALSE(command.captures_pointer({100, 100}));

    // When
    hold({200, 160});

    // Then: SiteWork sees neither the held button nor the release
    EXPECT_TRUE(command.captures_pointer({200, 160}));
}

TEST_F(UnitCommand, TheBoxIsDrawnOnlyWhileDragging) {
    // Given
    hold({100, 100});
    hold({200, 160});

    // When
    command.draw(canvas, Screen);
    const std::size_t while_dragging = canvas.fills.size();
    release({200, 160});
    command.draw(canvas, Screen);

    // Then
    EXPECT_GT(while_dragging, 0u);
    EXPECT_EQ(canvas.fills.size(), while_dragging);
}

TEST_F(UnitCommand, ALeftClickMakesThePlayerShootTheUnitUnderThePointer) {
    // Given: the soldier within the player's weapon range

    // When
    hold({soldier_on_screen.x, soldier_on_screen.y});
    release({soldier_on_screen.x, soldier_on_screen.y});
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_LT(model.registry().get<::world::Health>(soldier)->current, ::world::MaxHealth);
}

TEST_F(UnitCommand, ARightClickOnAUnitDoesNotShootIt) {
    // Given: nothing selected

    // When
    right_click({soldier_on_screen.x, soldier_on_screen.y});
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_EQ(model.registry().get<::world::Health>(soldier)->current, ::world::MaxHealth);
}

TEST_F(UnitCommand, ReleasingABoxSelectionDoesNotShoot) {
    // Given / When
    select_soldier();
    model.update(scene::world::SimulationTick);

    // Then
    EXPECT_EQ(model.registry().get<::world::Health>(soldier)->current, ::world::MaxHealth);
}

TEST_F(UnitCommand, HoldingTheButtonWithASemiAutomaticWeaponFiresOnlyOnce) {
    // Given: the player's starting Weapon is semi-automatic
    const input::PointerPosition at{soldier_on_screen.x, soldier_on_screen.y};

    // When: held for 2 seconds, past the cooldown
    for (int frame = 0; frame < 4; ++frame) {
        hold(at);
        model.update(0.5f);
    }

    // Then
    const int damage = ::world::weapon_stats(::world::ItemType::Weapon)->damage;
    EXPECT_EQ(model.registry().get<::world::Health>(soldier)->current, ::world::MaxHealth - damage);
}

TEST_F(UnitCommand, ClickingAgainAfterTheCooldownFiresAgain) {
    // Given
    const input::PointerPosition at{soldier_on_screen.x, soldier_on_screen.y};
    hold(at);
    release(at);
    model.update(1.1f);

    // When
    hold(at);
    release(at);
    model.update(scene::world::SimulationTick);

    // Then
    const int damage = ::world::weapon_stats(::world::ItemType::Weapon)->damage;
    EXPECT_EQ(model.registry().get<::world::Health>(soldier)->current, ::world::MaxHealth - 2 * damage);
}
