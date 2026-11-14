#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "scene/world/render/entity_renderer.hpp"
#include "support/fake_canvas.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/entities/character.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(EntityRenderer, CharacterIsDrawnAsACubeWithItsLabelAndStateAroundTheScreenCentre) {
    // Given
    core::Registry registry;
    const core::EntityId id = world::spawn_character(registry, "Player", {2.0f, 0.0f});
    registry.get<world::Character>(id)->state = world::CharacterState::Moving;
    const scene::world::EntityRenderer renderer(registry);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then: world (2, 0) with 64x32 tiles is (64, 32) pixels from the screen centre
    ASSERT_EQ(canvas.fills.size(), 2u); // top face, then front face
    const ui::Point marker_center = center_of(canvas.fills.back().rect);
    EXPECT_FLOAT_EQ(marker_center.x, 640.0f + 64.0f);
    EXPECT_FLOAT_EQ(marker_center.y, 360.0f + 32.0f);
    ASSERT_EQ(canvas.texts.size(), 1u);
    EXPECT_EQ(canvas.texts.front().text, "Player (Moving)");
    EXPECT_LT(canvas.texts.front().position.y, canvas.fills.front().rect.y);
}

TEST(EntityRenderer, EntityFurtherDownTheScreenIsDrawnAfterOneBehindIt) {
    // Given: the near entity is created first, so creation order cannot explain the result
    core::Registry registry;
    const core::EntityId near_id = registry.create("Near");
    registry.add(near_id, world::Position{{3.0f, 3.0f}});
    const core::EntityId far_id = registry.create("Far");
    registry.add(far_id, world::Position{{0.0f, 0.0f}});
    const scene::world::EntityRenderer renderer(registry);
    FakeCanvas canvas;

    // When
    renderer.draw(canvas, Screen);

    // Then
    ASSERT_EQ(canvas.texts.size(), 2u);
    EXPECT_EQ(canvas.texts[0].text, "Far");
    EXPECT_EQ(canvas.texts[1].text, "Near");
}
