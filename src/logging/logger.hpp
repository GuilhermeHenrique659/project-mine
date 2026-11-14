#pragma once

#include <functional>
#include <string>

namespace logging {

enum class Level { Info, Warning };

using Sink = std::function<void(Level, const std::string&)>;

// Replaces where messages go (default: stderr). Pass an empty Sink to restore the default.
void set_sink(Sink sink);

void info(const std::string& message);
void warning(const std::string& message);

} // namespace logging
