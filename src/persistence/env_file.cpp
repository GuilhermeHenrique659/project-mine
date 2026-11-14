#include "persistence/env_file.hpp"

#include <fstream>

namespace persistence {

namespace {

std::string trim(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

} // namespace

EnvValues read_env_file(const std::filesystem::path& file) {
    EnvValues values;
    std::ifstream input(file);
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }
        const auto separator = trimmed.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string name = trim(trimmed.substr(0, separator));
        if (name.empty()) {
            continue;
        }
        values[name] = trim(trimmed.substr(separator + 1));
    }
    return values;
}

std::optional<std::string> env_value(const std::string& name,
                                     const std::optional<std::string>& process_value,
                                     const EnvValues& file_values) {
    if (process_value) {
        return process_value;
    }
    const auto it = file_values.find(name);
    if (it == file_values.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace persistence
