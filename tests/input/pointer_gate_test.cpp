#include <gtest/gtest.h>

#include <memory>

#include "input/pointer_gate.hpp"
#include "support/fake_pointer.hpp"

TEST(PointerGate, AClaimedGateHidesTheButtonsAndWheelButKeepsThePosition) {
    // Given
    auto mouse = std::make_shared<FakePointer>();
    mouse->at = {10.0f, 20.0f};
    mouse->release = true;
    mouse->held = true;
    mouse->wheel = 2.0f;
    input::PointerGate gate(mouse);

    // When
    gate.claim();

    // Then
    EXPECT_FALSE(gate.released());
    EXPECT_FALSE(gate.down());
    EXPECT_FLOAT_EQ(gate.scroll(), 0.0f);
    EXPECT_FLOAT_EQ(gate.position().x, 10.0f);
}

TEST(PointerGate, ResetPassesThePointerThroughAgain) {
    // Given
    auto mouse = std::make_shared<FakePointer>();
    mouse->release = true;
    input::PointerGate gate(mouse);
    gate.claim();

    // When
    gate.reset();

    // Then
    EXPECT_TRUE(gate.released());
}
