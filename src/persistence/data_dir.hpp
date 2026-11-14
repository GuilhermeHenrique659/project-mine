#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace persistence {

// Directory where user data is stored. Priority: `--data-dir <path>` argument,
// then the PROJECT_MINE_DATA_DIR value, then the build default.
std::filesystem::path resolve_data_dir(const std::vector<std::string>& args,
                                       const std::optional<std::string>& env_value);

// Dev (Debug) builds use `resource/`; release builds use %APPDATA%/ProjectMine.
std::filesystem::path default_data_dir();

} // namespace persistence
