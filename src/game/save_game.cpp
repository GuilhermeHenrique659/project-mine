#include "game/save_game.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <random>

namespace game {

namespace {

bool is_allowed(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == ' ' || c == '-' || c == '_';
}

} // namespace

std::string normalize_save_name(const std::string& name) {
    const auto first = name.find_first_not_of(' ');
    if (first == std::string::npos) {
        return {};
    }
    const auto last = name.find_last_not_of(' ');
    return name.substr(first, last - first + 1);
}

bool is_valid_save_name(const std::string& name) {
    if (name.empty() || name.size() > MaxSaveNameLength) {
        return false;
    }
    if (name.front() == ' ' || name.back() == ' ') {
        return false;
    }
    return std::all_of(name.begin(), name.end(), is_allowed);
}

std::string format_created_at(std::int64_t created_at) {
    const std::time_t time = static_cast<std::time_t>(created_at);
    const std::tm* local = std::localtime(&time);
    if (local == nullptr) {
        return {};
    }
    char buffer[32];
    const std::size_t written = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", local);
    return std::string(buffer, written);
}

std::uint64_t random_seed() {
    std::random_device device;
    return (static_cast<std::uint64_t>(device()) << 32) | device();
}

std::int64_t system_clock_seconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}

} // namespace game
