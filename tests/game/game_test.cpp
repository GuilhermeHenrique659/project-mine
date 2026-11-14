#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>

#include "game/display_settings.hpp"
#include "game/game.hpp"
#include "game/game_events.hpp"
#include "persistence/settings_store.hpp"
#include "support/temp_dir.hpp"

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

TEST(Game, SettingsRequestedSwitchesToAnotherScene) {
    // Given
    TempDir dir("game_test");
    game::Game game(dir.path());
    int updates = 0;
    game.change_scene(make_counting_scene(updates));
    game.update(0.016f);
    const scene::Scene* before = game.active_scene();

    // When
    game.event_bus().publish(game::SettingsRequested{});
    game.update(0.016f);

    // Then
    ASSERT_NE(game.active_scene(), nullptr);
    EXPECT_NE(game.active_scene(), before);
}

TEST(Game, DisplaySettingsChangedIsRememberedForTheNextSettingsScreen) {
    // Given
    TempDir dir("game_test");
    game::Game game(dir.path());
    const game::DisplaySettings chosen{game::resolution_presets()[2], true};

    // When
    game.event_bus().publish(game::DisplaySettingsChanged{chosen});

    // Then
    EXPECT_EQ(game.display_settings(), chosen);
}

TEST(Game, ChangeSceneIsAppliedOnNextUpdate) {
    // Given
    TempDir dir("game_test");
    game::Game game(dir.path());
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

TEST(Game, StartsWithSettingsSavedByAPreviousRun) {
    // Given
    TempDir dir("game_restore");
    const game::DisplaySettings saved{game::resolution_presets()[2], true};
    persistence::SettingsStore(dir.path()).save(saved);

    // When
    game::Game game(dir.path());

    // Then
    EXPECT_EQ(game.display_settings(), saved);
}

TEST(Game, ChosenSettingsSurviveARestart) {
    // Given
    TempDir dir("game_restart");
    const game::DisplaySettings chosen{game::resolution_presets()[1], true};
    {
        game::Game first_run(dir.path());
        first_run.event_bus().publish(game::DisplaySettingsChanged{chosen});
    }

    // When
    game::Game second_run(dir.path());

    // Then
    EXPECT_EQ(second_run.display_settings(), chosen);
}

TEST(Game, CorruptSettingsFileFallsBackToDefaults) {
    // Given
    TempDir dir("game_corrupt");
    std::filesystem::create_directories(dir.path());
    std::ofstream(dir.path() / "settings.cfg") << "resolution_width=-5\n";

    // When
    game::Game game(dir.path());

    // Then
    EXPECT_EQ(game.display_settings(), game::DisplaySettings{});
}
