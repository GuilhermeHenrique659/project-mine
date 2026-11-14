#include "persistence/save_store.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <system_error>

#include "logging/logger.hpp"
#include "persistence/atomic_file.hpp"
#include "text/parse.hpp"

namespace persistence {

namespace {

constexpr int FormatVersion = 2;
constexpr const char* EntitySection = "[entity]";
constexpr const char* SaveExtension = ".sav";
constexpr const char* BackupExtension = ".sav_old";

// Empty when the file is missing, has no valid creation time or comes from a newer format
// (loading it would drop what this build does not know, and the next save would lose it).
// The save takes the name it was asked for (the file name), which is its identity.
// Layout: the header keys first, then one "[entity]" section per entity; every key after
// an "[entity]" line belongs to that entity.
std::optional<game::SaveGame> read_save(const std::filesystem::path& file,
                                        const std::string& name) {
    std::ifstream input(file);
    if (!input) {
        return std::nullopt;
    }

    std::optional<std::int64_t> created_at;
    bool newer_version = false;
    // Saves written before worlds had seeds have none: they get seed 0.
    std::uint64_t seed = 0;
    std::vector<game::EntityRecord> entities;
    bool in_entity = false;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == EntitySection) {
            in_entity = true;
            entities.emplace_back();
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (in_entity) {
            game::EntityRecord& record = entities.back();
            if (key == "archetype") {
                record.archetype = value;
            } else if (key == "label") {
                record.label = value;
            } else {
                record.fields[key] = value;
            }
        } else if (key == "version") {
            // Unreadable versions count as newer: better refuse than lose data.
            newer_version = text::parse<int>(value).value_or(FormatVersion + 1) > FormatVersion;
        } else if (key == "created_at") {
            created_at = text::parse<std::int64_t>(value);
        } else if (key == "seed") {
            seed = text::parse<std::uint64_t>(value).value_or(0);
        }
    }
    if (newer_version) {
        logging::warning("Save " + file.string() + " was written by a newer version of the game");
        return std::nullopt;
    }
    if (!created_at) {
        return std::nullopt;
    }
    // A record that does not say what it is cannot be respawned.
    std::erase_if(entities,
                  [](const game::EntityRecord& record) { return record.archetype.empty(); });
    return game::SaveGame{name, *created_at, seed, std::move(entities)};
}

// True when the text fits on one line of the file.
bool is_single_line(const std::string& text) {
    return text.find_first_of("\r\n") == std::string::npos;
}

// One "[entity]" section. Keys and values that would break the line format are skipped.
std::string entity_section(const game::EntityRecord& record) {
    if (record.archetype.empty() || !is_single_line(record.archetype) ||
        !is_single_line(record.label)) {
        logging::warning("Skipping an entity that cannot be written to the save");
        return {};
    }
    std::string section = std::string(EntitySection) + "\narchetype=" + record.archetype +
                          "\nlabel=" + record.label + "\n";
    for (const auto& [key, value] : record.fields) {
        // "archetype" and "label" would be read back as the record's own header.
        if (key.empty() || key == "archetype" || key == "label" ||
            key.find('=') != std::string::npos || !is_single_line(key) || !is_single_line(value)) {
            logging::warning("Skipping unwritable entity field \"" + key + "\"");
            continue;
        }
        section += key + "=" + value + "\n";
    }
    return section;
}

} // namespace

SaveStore::SaveStore(std::filesystem::path data_dir) : directory_(std::move(data_dir) / "saves") {}

bool SaveStore::save(const game::SaveGame& save) const {
    if (!game::is_valid_save_name(save.name)) {
        logging::warning("Could not save game, invalid name \"" + save.name + "\"");
        return false;
    }

    const std::filesystem::path file = file_of(save.name);
    std::string content = "version=" + std::to_string(FormatVersion) + "\n" +
                          "name=" + save.name + "\n" +
                          "created_at=" + std::to_string(save.created_at) + "\n" +
                          "seed=" + std::to_string(save.seed) + "\n";
    for (const game::EntityRecord& record : save.entities) {
        content += entity_section(record);
    }
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
