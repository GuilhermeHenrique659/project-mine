#include "logging/logger.hpp"

#include <iostream>

namespace logging {

namespace {

Sink g_sink;

void write(Level level, const std::string& message) {
    if (g_sink) {
        g_sink(level, message);
        return;
    }
    const char* tag = level == Level::Warning ? "[WARN] " : "[INFO] ";
    std::cerr << tag << message << '\n';
}

} // namespace

void set_sink(Sink sink) { g_sink = std::move(sink); }

void info(const std::string& message) { write(Level::Info, message); }

void warning(const std::string& message) { write(Level::Warning, message); }

} // namespace logging
