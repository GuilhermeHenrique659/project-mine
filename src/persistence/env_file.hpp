#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace persistence {

using EnvValues = std::unordered_map<std::string, std::string>;

// Reads `NAME=value` lines from a .env file. Blank lines, `#` comments and lines without
// `=` are skipped. A missing file gives an empty map.
EnvValues read_env_file(const std::filesystem::path& file);

// The real environment variable wins; the .env value is the fallback.
std::optional<std::string> env_value(const std::string& name,
                                     const std::optional<std::string>& process_value,
                                     const EnvValues& file_values);

} // namespace persistence
