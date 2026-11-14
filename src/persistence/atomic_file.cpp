#include "persistence/atomic_file.hpp"

#include <fstream>
#include <system_error>

namespace persistence {

namespace {

bool write_content(const std::filesystem::path& file, const std::string& content) {
    std::ofstream output(file, std::ios::trunc | std::ios::binary);
    if (!output) {
        return false;
    }
    output << content;
    output.flush();
    return static_cast<bool>(output);
}

} // namespace

std::optional<std::string> write_file_atomically(const std::filesystem::path& file,
                                                 const std::string& content,
                                                 const std::filesystem::path& backup) {
    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    if (error) {
        return "cannot create " + file.parent_path().string() + ": " + error.message();
    }

    const std::filesystem::path temporary = file.string() + ".tmp";
    if (!write_content(temporary, content)) {
        std::filesystem::remove(temporary, error);
        return "cannot write " + temporary.string();
    }

    if (!backup.empty() && std::filesystem::exists(file, error)) {
        std::filesystem::rename(file, backup, error);
        if (error) {
            const std::string reason = "cannot back up " + file.string() + ": " + error.message();
            std::filesystem::remove(temporary, error);
            return reason;
        }
    }

    std::filesystem::rename(temporary, file, error);
    if (error) {
        const std::string reason = "cannot replace " + file.string() + ": " + error.message();
        std::filesystem::remove(temporary, error);
        return reason;
    }
    return std::nullopt;
}

} // namespace persistence
