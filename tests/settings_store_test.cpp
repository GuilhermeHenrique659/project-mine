#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "persistence/settings_store.hpp"
#include "temp_dir.hpp"

TEST(SettingsStore, SavedSettingsAreLoadedBack) {
    // Given
    TempDir dir("store_roundtrip");
    persistence::SettingsStore store(dir.path() / "nested");
    const game::DisplaySettings settings{game::resolution_presets()[2], true};

    // When
    const bool saved = store.save(settings);

    // Then
    ASSERT_TRUE(saved);
    const auto loaded = store.load();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, settings);
}

TEST(SettingsStore, MissingFileLoadsNothing) {
    // Given
    TempDir dir("store_missing");
    persistence::SettingsStore store(dir.path());

    // When
    const auto loaded = store.load();

    // Then
    EXPECT_FALSE(loaded.has_value());
}

TEST(SettingsStore, NonPositiveResolutionIsRejected) {
    // Given
    TempDir dir("store_invalid");
    persistence::SettingsStore store(dir.path());
    std::filesystem::create_directories(dir.path());
    std::ofstream(store.file()) << "resolution_width=0\nresolution_height=720\nfullscreen=1\n";

    // When
    const auto loaded = store.load();

    // Then
    EXPECT_FALSE(loaded.has_value());
}

TEST(SettingsStore, UnknownKeysAndGarbageLinesAreIgnored) {
    // Given
    TempDir dir("store_unknown");
    persistence::SettingsStore store(dir.path());
    std::filesystem::create_directories(dir.path());
    std::ofstream(store.file())
        << "version=1\nfuture_option=7\nnot a pair\nresolution_width=1920\n"
           "resolution_height=1080\nfullscreen=1\n";

    // When
    const auto loaded = store.load();

    // Then
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, (game::DisplaySettings{{1920, 1080}, true}));
}

TEST(SettingsStore, SaveLeavesNoTemporaryFileBehind) {
    // Given
    TempDir dir("store_tmp");
    persistence::SettingsStore store(dir.path());

    // When
    store.save(game::DisplaySettings{});

    // Then
    EXPECT_TRUE(std::filesystem::exists(store.file()));
    EXPECT_FALSE(std::filesystem::exists(store.file().string() + ".tmp"));
}
