#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "input/input_handler.hpp"
#include "input/input_map.hpp"
#include "support/fake_keyboard.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/systems/player_input_system.hpp"

namespace {

core::EntityId spawn_player(core::Registry& registry) {
    const core::EntityId id = registry.create("player");
    registry.add(id, world::PlayerControlled{});
    registry.add(id, world::Mover{4.0f, {0.0f, 0.0f}});
    return id;
}

world::WorldPoint direction_of(const core::Registry& registry, core::EntityId id) {
    return registry.get<world::Mover>(id)->direction;
}

} // namespace

TEST(PlayerInputSystem, HeldUpWalksTowardsTheTopOfTheScreen) {
    // Given
    core::Registry registry;
    FakeKeyboard keyboard;
    input::InputHandler input(keyboard, input::InputMap::world_defaults());
    world::PlayerInputSystem system(registry, input);
    const core::EntityId id = spawn_player(registry);
    keyboard.down = {input::Key::W};

    // When
    system.update(0.016f);

    // Then: screen up is world (-1, -1)
    EXPECT_FLOAT_EQ(direction_of(registry, id).x, -1.0f);
    EXPECT_FLOAT_EQ(direction_of(registry, id).y, -1.0f);
}

TEST(PlayerInputSystem, HeldKeysCombineIntoOneDirection) {
    // Given
    core::Registry registry;
    FakeKeyboard keyboard;
    input::InputHandler input(keyboard, input::InputMap::world_defaults());
    world::PlayerInputSystem system(registry, input);
    const core::EntityId id = spawn_player(registry);
    keyboard.down = {input::Key::W, input::Key::D};

    // When
    system.update(0.016f);

    // Then: screen up-right (1, -1) is world (0, -2)
    EXPECT_FLOAT_EQ(direction_of(registry, id).x, 0.0f);
    EXPECT_FLOAT_EQ(direction_of(registry, id).y, -2.0f);
}

TEST(PlayerInputSystem, OppositeKeysCancelEachOther) {
    // Given
    core::Registry registry;
    FakeKeyboard keyboard;
    input::InputHandler input(keyboard, input::InputMap::world_defaults());
    world::PlayerInputSystem system(registry, input);
    const core::EntityId id = spawn_player(registry);
    keyboard.down = {input::Key::A, input::Key::D};

    // When
    system.update(0.016f);

    // Then
    EXPECT_FLOAT_EQ(direction_of(registry, id).x, 0.0f);
    EXPECT_FLOAT_EQ(direction_of(registry, id).y, 0.0f);
}

TEST(PlayerInputSystem, ReleasingAllKeysStopsThePlayer) {
    // Given
    core::Registry registry;
    FakeKeyboard keyboard;
    input::InputHandler input(keyboard, input::InputMap::world_defaults());
    world::PlayerInputSystem system(registry, input);
    const core::EntityId id = spawn_player(registry);
    keyboard.down = {input::Key::S};
    system.update(0.016f);

    // When
    keyboard.down.clear();
    system.update(0.016f);

    // Then
    EXPECT_FLOAT_EQ(direction_of(registry, id).x, 0.0f);
    EXPECT_FLOAT_EQ(direction_of(registry, id).y, 0.0f);
}

TEST(PlayerInputSystem, HoldingShiftMakesThePlayerRun) {
    // Given
    core::Registry registry;
    FakeKeyboard keyboard;
    input::InputHandler input(keyboard, input::InputMap::world_defaults());
    world::PlayerInputSystem system(registry, input);
    const core::EntityId id = spawn_player(registry);
    keyboard.down = {input::Key::W, input::Key::LeftShift};

    // When
    system.update(0.016f);

    // Then
    EXPECT_TRUE(registry.get<world::Mover>(id)->running);
}

TEST(PlayerInputSystem, ReleasingShiftMakesThePlayerWalkAgain) {
    // Given
    core::Registry registry;
    FakeKeyboard keyboard;
    input::InputHandler input(keyboard, input::InputMap::world_defaults());
    world::PlayerInputSystem system(registry, input);
    const core::EntityId id = spawn_player(registry);
    keyboard.down = {input::Key::W, input::Key::LeftShift};
    system.update(0.016f);

    // When
    keyboard.down = {input::Key::W};
    system.update(0.016f);

    // Then
    EXPECT_FALSE(registry.get<world::Mover>(id)->running);
}

TEST(PlayerInputSystem, EntityThatIsNotPlayerControlledIgnoresTheInput) {
    // Given
    core::Registry registry;
    FakeKeyboard keyboard;
    input::InputHandler input(keyboard, input::InputMap::world_defaults());
    world::PlayerInputSystem system(registry, input);
    const core::EntityId id = registry.create("cart");
    registry.add(id, world::Mover{4.0f, {0.0f, 0.0f}});
    keyboard.down = {input::Key::W};

    // When
    system.update(0.016f);

    // Then
    EXPECT_FLOAT_EQ(direction_of(registry, id).x, 0.0f);
    EXPECT_FLOAT_EQ(direction_of(registry, id).y, 0.0f);
}
