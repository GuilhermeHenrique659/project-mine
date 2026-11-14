#pragma once

#include <filesystem>
#include <optional>

#include "game/display_settings.hpp"

namespace persistence {

// Reads and writes the display settings as a `key=value` text file inside a data directory.
class SettingsStore {
public:
    explicit SettingsStore(std::filesystem::path data_dir);

    // Empty when the file is missing or holds invalid values.
    std::optional<game::DisplaySettings> load() const;

    // Writes through a temporary file so a crash never leaves a half-written settings file.
    bool save(const game::DisplaySettings& settings) const;

    const std::filesystem::path& file() const { return file_; }

private:
    std::filesystem::path file_;
};

} // namespace persistence
