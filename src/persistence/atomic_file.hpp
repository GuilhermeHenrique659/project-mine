#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace persistence {

// Replaces `file` with `content` without ever leaving it half-written: the content goes to
// `<file>.tmp` first and is renamed over the target. Parent directories are created.
// When `backup` is given and `file` already exists, the existing file is moved to `backup`
// (replacing an older backup) before the new one takes its place.
// Returns a description of what went wrong, or empty on success.
std::optional<std::string> write_file_atomically(const std::filesystem::path& file,
                                                 const std::string& content,
                                                 const std::filesystem::path& backup = {});

} // namespace persistence
