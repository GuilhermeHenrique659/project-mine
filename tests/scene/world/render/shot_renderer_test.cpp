#include <gtest/gtest.h>

#include "game/save_game.hpp"
#include "scene/world/render/shot_renderer.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

game::SaveGame save_with_soldier() {
    game::SaveGame save{"Alpha", 0};
    save.entities.push_back(
        {"soldier", "Soldier", {{"position.x", "2.5"}, {"position.y", "2.5"}}});
    return save;
}

} // namespace

TEST(ShotRenderer, DrawsAFiredShotAsALineUntilItFades) {
    // Given
    event::EventBus bus;
    FakeInput input;
    scene::world::WorldModel model(bus, input.handler, save_with_soldier());
    const scene::world::ShotRenderer renderer(model);
    model.shoot_at({2, 2});
    model.update(scene::world::SimulationTick);

    // When
    FakeCanvas during;
    renderer.draw(during, Screen);
    model.update(scene::world::ShotTraceDuration);
    FakeCanvas after;
    renderer.draw(after, Screen);

    // Then: a line is a quad of two triangles
    EXPECT_EQ(during.triangles.size(), 2u);
    EXPECT_TRUE(after.triangles.empty());
}
