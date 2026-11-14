#include "persistence/data_dir.hpp"

#include <cstdlib>

namespace persistence {

std::filesystem::path resolve_data_dir(const std::vector<std::string>& args,
                                       const std::optional<std::string>& env_value) {
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "--data-dir") {
            return args[i + 1];
        }
    }
    if (env_value && !env_value->empty()) {
        return *env_value;
    }
    return default_data_dir();
}

std::filesystem::path default_data_dir() {
#ifdef PROJECT_MINE_DEV
    // Relative on purpose: the project path has an accent. Run from the project root.
    return "resource";
#else
    const char* app_data = std::getenv("APPDATA");
    if (!app_data || !*app_data) {
        return ".";
    }
    return std::filesystem::path(app_data) / "ProjectMine";
#endif
}

} // namespace persistence
