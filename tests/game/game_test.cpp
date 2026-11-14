#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>

#include "game/display_settings.hpp"
#include "game/game.hpp"
#include "game/game_events.hpp"
#include "persistence/save_store.hpp"
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

namespace {

constexpr std::int64_t FixedNow = 1'700'000'000;

std::int64_t fixed_clock() { return FixedNow; }

// A game already showing a known scene, so a scene switch can be observed.
struct RunningGame {
    explicit RunningGame(const std::string& dir_name)
        : dir(dir_name), game(dir.path(), fixed_clock) {
        game.change_scene(make_counting_scene(updates));
        game.update(0.016f);
        initial_scene = game.active_scene();
    }

    bool switched_scene() {
        game.update(0.016f);
        return game.active_scene() != nullptr && game.active_scene() != initial_scene;
    }

    TempDir dir;
    game::Game game;
    int updates = 0;
    const scene::Scene* initial_scene = nullptr;
};

} // namespace

TEST(Game, NewGameRequestedOpensTheNamePrompt) {
    // Given
    RunningGame running("game_new_prompt");

    // When
    running.game.event_bus().publish(game::NewGameRequested{});

    // Then: another screen is shown and nothing was saved yet
    EXPECT_TRUE(running.switched_scene());
    EXPECT_TRUE(running.game.save_store().list().empty());
}

TEST(Game, LoadGameRequestedOpensTheSaveList) {
    // Given
    RunningGame running("game_load_list");

    // When
    running.game.event_bus().publish(game::LoadGameRequested{});

    // Then
    EXPECT_TRUE(running.switched_scene());
}

TEST(Game, CreateGameRequestedSavesTheNewGameAndStartsPlaying) {
    // Given
    RunningGame running("game_create");

    // When
    running.game.event_bus().publish(game::CreateGameRequested{"My Base"});

    // Then
    const auto saved = running.game.save_store().load("My Base");
    ASSERT_TRUE(saved.has_value());
    EXPECT_EQ(*saved, (game::SaveGame{"My Base", FixedNow}));
    EXPECT_TRUE(running.switched_scene());
}

TEST(Game, CreateGameRequestedWithAnInvalidNameSavesNothingAndStaysOnTheScreen) {
    // Given
    RunningGame running("game_create_invalid");

    // When
    running.game.event_bus().publish(game::CreateGameRequested{"../escape"});

    // Then
    EXPECT_TRUE(running.game.save_store().list().empty());
    EXPECT_FALSE(running.switched_scene());
}

TEST(Game, CreatingAGameWithAnExistingNameOverwritesTheOldSave) {
    // Given
    RunningGame running("game_create_overwrite");
    persistence::SaveStore(running.dir.path()).save({"base", 100});

    // When
    running.game.event_bus().publish(game::CreateGameRequested{"base"});

    // Then
    const auto saves = running.game.save_store().list();
    ASSERT_EQ(saves.size(), 1u);
    EXPECT_EQ(saves[0], (game::SaveGame{"base", FixedNow}));
}

TEST(Game, LoadSaveRequestedForAnExistingSaveStartsPlaying) {
    // Given
    RunningGame running("game_load");
    persistence::SaveStore(running.dir.path()).save({"base", 100});

    // When
    running.game.event_bus().publish(game::LoadSaveRequested{"base"});

    // Then: playing, and loading did not rewrite the save
    EXPECT_TRUE(running.switched_scene());
    const auto saved = running.game.save_store().load("base");
    ASSERT_TRUE(saved.has_value());
    EXPECT_EQ(saved->created_at, 100);
}

TEST(Game, LoadSaveRequestedForAMissingSaveStaysOnTheScreen) {
    // Given
    RunningGame running("game_load_missing");

    // When
    running.game.event_bus().publish(game::LoadSaveRequested{"ghost"});

    // Then
    EXPECT_FALSE(running.switched_scene());
}

TEST(Game, SaveGameRequestedWritesTheGameToDisk) {
    // Given
    RunningGame running("game_save");

    // When
    running.game.event_bus().publish(game::SaveGameRequested{game::SaveGame{"base", 42}});

    // Then: saved, and the player keeps playing
    const auto saved = running.game.save_store().load("base");
    ASSERT_TRUE(saved.has_value());
    EXPECT_EQ(saved->created_at, 42);
    EXPECT_FALSE(running.switched_scene());
}
