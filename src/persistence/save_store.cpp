#include "persistence/save_store.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <system_error>

#include "logging/logger.hpp"
#include "persistence/atomic_file.hpp"

namespace persistence {

namespace {

constexpr int FormatVersion = 1;
constexpr const char* SaveExtension = ".sav";
constexpr const char* BackupExtension = ".sav_old";

// Parses a whole non-negative integer; empty when the text has anything else.
std::optional<std::int64_t> parse_int(const std::string& text) {
    if (text.empty() || text.size() > 18) {
        return std::nullopt;
    }
    std::int64_t value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        value = value * 10 + (c - '0');
    }
    return value;
}

// Parses a whole unsigned 64-bit integer; empty when the text has anything else or overflows.
std::optional<std::uint64_t> parse_u64(const std::string& text) {
    if (text.empty() || text.size() > 20) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        const auto digit = static_cast<std::uint64_t>(c - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
            return std::nullopt;
        }
        value = value * 10 + digit;
    }
    return value;
}

// Empty when the file is missing or has no valid creation time. The save takes the name
// it was asked for (the file name), which is its identity.
std::optional<game::SaveGame> read_save(const std::filesystem::path& file,
                                        const std::string& name) {
    std::ifstream input(file);
    if (!input) {
        return std::nullopt;
    }

    std::optional<std::int64_t> created_at;
    // Saves written before worlds had seeds have none: they get seed 0.
    std::uint64_t seed = 0;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        if (line.substr(0, separator) == "created_at") {
            created_at = parse_int(line.substr(separator + 1));
        } else if (line.substr(0, separator) == "seed") {
            seed = parse_u64(line.substr(separator + 1)).value_or(0);
        }
    }
    if (!created_at) {
        return std::nullopt;
    }
    return game::SaveGame{name, *created_at, seed};
}

} // namespace

SaveStore::SaveStore(std::filesystem::path data_dir) : directory_(std::move(data_dir) / "saves") {}

bool SaveStore::save(const game::SaveGame& save) const {
    if (!game::is_valid_save_name(save.name)) {
        logging::warning("Could not save game, invalid name \"" + save.name + "\"");
        return false;
    }

    const std::filesystem::path file = file_of(save.name);
    const std::string content = "version=" + std::to_string(FormatVersion) + "\n" +
                                "name=" + save.name + "\n" +
                                "created_at=" + std::to_string(save.created_at) + "\n" +
                                "seed=" + std::to_string(save.seed) + "\n";
    const auto failure = write_file_atomically(file, content, backup_of(save.name));
    if (failure) {
        logging::warning("Could not save game \"" + save.name + "\": " + *failure);
        return false;
    }
    logging::info("Saved game to " + file.string());
    return true;
}

std::optional<game::SaveGame> SaveStore::load(const std::string& name) const {
    if (!game::is_valid_save_name(name)) {
        logging::warning("Could not load game, invalid name \"" + name + "\"");
        return std::nullopt;
    }

    const std::filesystem::path file = file_of(name);
    if (auto save = read_save(file, name)) {
        logging::info("Loaded game from " + file.string());
        return save;
    }

    const std::filesystem::path backup = backup_of(name);
    if (auto save = read_save(backup, name)) {
        logging::warning("Save " + file.string() + " is missing or corrupt, loaded backup " +
                         backup.string());
        return save;
    }

    logging::warning("Could not load game \"" + name + "\" from " + file.string());
    return std::nullopt;
}

std::vector<game::SaveGame> SaveStore::list() const {
    std::error_code error;
    std::filesystem::directory_iterator entries(directory_, error);
    if (error) {
        return {};
    }

    // A save may exist only as a backup (its current file was lost), so both count.
    std::set<std::string> names;
    for (const std::filesystem::directory_entry& entry : entries) {
        const std::string extension = entry.path().extension().string();
        if (extension != SaveExtension && extension != BackupExtension) {
            continue;
        }
        const std::string name = entry.path().stem().string();
        if (game::is_valid_save_name(name)) {
            names.insert(name);
        }
    }

    std::vector<game::SaveGame> saves;
    for (const std::string& name : names) {
        if (auto save = load(name)) {
            saves.push_back(std::move(*save));
        }
    }
    std::stable_sort(saves.begin(), saves.end(),
                     [](const game::SaveGame& left, const game::SaveGame& right) {
                         return left.created_at > right.created_at;
                     });
    return saves;
}

std::filesystem::path SaveStore::file_of(const std::string& name) const {
    return directory_ / (name + SaveExtension);
}

std::filesystem::path SaveStore::backup_of(const std::string& name) const {
    return directory_ / (name + BackupExtension);
}

} // namespace persistence
