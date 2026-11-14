#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "game/save_game.hpp"

namespace persistence {

// Reads and writes saved games as `key=value` text files in `<data_dir>/saves/<name>.sav`.
// Every save keeps the version it replaces as `<name>.sav_old`, so a file corrupted while
// being written can still be recovered from the previous one.
class SaveStore {
public:
    explicit SaveStore(std::filesystem::path data_dir);

    // Writes the save, moving the existing file (if any) to `<name>.sav_old` first and
    // replacing an older backup. False when the name is invalid or the disk write fails.
    bool save(const game::SaveGame& save) const;

    // Reads `<name>.sav`, falling back to `<name>.sav_old` when it is missing or corrupt.
    // Empty when neither can be read.
    std::optional<game::SaveGame> load(const std::string& name) const;

    // Every loadable save, newest first.
    std::vector<game::SaveGame> list() const;

    const std::filesystem::path& directory() const { return directory_; }
    std::filesystem::path file_of(const std::string& name) const;
    std::filesystem::path backup_of(const std::string& name) const;

private:
    std::filesystem::path directory_;
};

} // namespace persistence
