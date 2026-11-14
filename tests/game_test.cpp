#include <gtest/gtest.h>

#include <memory>

#include "game/game.hpp"

namespace {

class CountingModel : public scene::Model {
public:
    explicit CountingModel(int& updates) : updates_(updates) {}
    void update(float) override { ++updates_; }

private:
    int& updates_;
};

std::unique_ptr<scene::Scene> make_counting_scene(int& updates) {
    return std::make_unique<scene::Scene>(std::make_unique<CountingModel>(updates), nullptr,
                                          nullptr);
}

} // namespace

TEST(Game, ChangeSceneIsAppliedOnNextUpdate) {
    // Given
    game::Game game;
    int first_updates = 0;
    int second_updates = 0;
    game.change_scene(make_counting_scene(first_updates));
    game.update(0.016f);
    const scene::Scene* first_scene = game.active_scene();

    // When
    game.change_scene(make_counting_scene(second_updates));

    // Then
    EXPECT_EQ(game.active_scene(), first_scene);
    game.update(0.016f);
    EXPECT_NE(game.active_scene(), first_scene);
    EXPECT_EQ(first_updates, 1);
    EXPECT_EQ(second_updates, 1);
}
