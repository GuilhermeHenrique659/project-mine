#include "persistence/settings_store.hpp"

#include <fstream>
#include <string>
#include <system_error>

#include "logging/logger.hpp"

namespace persistence {

namespace {

constexpr int FormatVersion = 1;

// Parses a whole non-negative integer; empty when the text has anything else.
std::optional<int> parse_int(const std::string& text) {
    if (text.empty() || text.size() > 9) {
        return std::nullopt;
    }
    int value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        value = value * 10 + (c - '0');
    }
    return value;
}

} // namespace

SettingsStore::SettingsStore(std::filesystem::path data_dir)
    : file_(std::move(data_dir) / "settings.cfg") {}

std::optional<game::DisplaySettings> SettingsStore::load() const {
    std::ifstream input(file_);
    if (!input) {
        logging::info("No settings file at " + file_.string() + ", using defaults");
        return std::nullopt;
    }

    game::DisplaySettings settings;
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, separator);
        const auto value = parse_int(line.substr(separator + 1));
        if (!value) {
            continue;
        }
        if (key == "resolution_width") {
            settings.resolution.width = *value;
        } else if (key == "resolution_height") {
            settings.resolution.height = *value;
        } else if (key == "fullscreen") {
            settings.fullscreen = *value != 0;
        }
    }

    if (settings.resolution.width <= 0 || settings.resolution.height <= 0) {
        logging::warning("Invalid settings in " + file_.string() + ", using defaults");
        return std::nullopt;
    }
    logging::info("Loaded settings from " + file_.string());
    return settings;
}

bool SettingsStore::save(const game::DisplaySettings& settings) const {
    std::error_code error;
    std::filesystem::create_directories(file_.parent_path(), error);
    if (error) {
        logging::warning("Could not save settings, cannot create " +
                         file_.parent_path().string() + ": " + error.message());
        return false;
    }

    const std::filesystem::path temporary = file_.string() + ".tmp";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) {
            logging::warning("Could not save settings, cannot write " + temporary.string());
            return false;
        }
        output << "version=" << FormatVersion << '\n'
               << "resolution_width=" << settings.resolution.width << '\n'
               << "resolution_height=" << settings.resolution.height << '\n'
               << "fullscreen=" << (settings.fullscreen ? 1 : 0) << '\n';
        if (!output) {
            logging::warning("Could not save settings, write failed for " + temporary.string());
            return false;
        }
    }

    std::filesystem::rename(temporary, file_, error);
    if (error) {
        logging::warning("Could not save settings to " + file_.string() + ": " + error.message());
        std::filesystem::remove(temporary, error);
        return false;
    }
    logging::info("Saved settings to " + file_.string());
    return true;
}

} // namespace persistence
