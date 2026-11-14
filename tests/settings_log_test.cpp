#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "logging/logger.hpp"
#include "persistence/settings_store.hpp"
#include "temp_dir.hpp"

namespace {

// Captures log messages for the test and restores the default sink afterwards.
class LogCapture {
public:
    LogCapture() {
        logging::set_sink([this](logging::Level level, const std::string& message) {
            entries.emplace_back(level, message);
        });
    }
    ~LogCapture() { logging::set_sink({}); }

    bool has(logging::Level level, const std::string& text) const {
        for (const auto& [entry_level, message] : entries) {
            if (entry_level == level && message.find(text) != std::string::npos) return true;
        }
        return false;
    }

    std::vector<std::pair<logging::Level, std::string>> entries;
};

} // namespace

TEST(SettingsLog, SuccessfulSaveIsLoggedWithTheFilePath) {
    // Given
    TempDir dir("log_save");
    LogCapture log;
    persistence::SettingsStore store(dir.path());

    // When
    store.save(game::DisplaySettings{});

    // Then
    EXPECT_TRUE(log.has(logging::Level::Info, "settings.cfg"));
}

TEST(SettingsLog, FailedSaveIsLoggedAsWarning) {
    // Given
    TempDir dir("log_save_fail");
    std::ofstream(dir.path());  // a file where the data directory should be
    LogCapture log;
    persistence::SettingsStore store(dir.path());

    // When
    const bool saved = store.save(game::DisplaySettings{});

    // Then
    EXPECT_FALSE(saved);
    EXPECT_TRUE(log.has(logging::Level::Warning, "Could not save"));
}

TEST(SettingsLog, MissingFileIsLoggedAsInfoNotWarning) {
    // Given
    TempDir dir("log_missing");
    LogCapture log;

    // When
    persistence::SettingsStore(dir.path()).load();

    // Then
    EXPECT_TRUE(log.has(logging::Level::Info, "using defaults"));
    EXPECT_FALSE(log.has(logging::Level::Warning, ""));
}

TEST(SettingsLog, InvalidFileIsLoggedAsWarning) {
    // Given
    TempDir dir("log_invalid");
    persistence::SettingsStore store(dir.path());
    std::filesystem::create_directories(dir.path());
    std::ofstream(store.file()) << "resolution_width=0\n";
    LogCapture log;

    // When
    store.load();

    // Then
    EXPECT_TRUE(log.has(logging::Level::Warning, "Invalid settings"));
}
