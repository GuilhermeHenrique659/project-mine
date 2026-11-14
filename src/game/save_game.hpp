#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace game {

constexpr std::size_t MaxSaveNameLength = 32;

// The state of one saved game. Its identity and the seed of its world; the rest of the world state is added later.
struct SaveGame {
    std::string name;
    // Unix time (seconds) of when the game was created.
    std::int64_t created_at = 0;
    // Seed the world is generated from; the same seed recreates the same world.
    std::uint64_t seed = 0;

    bool operator==(const SaveGame&) const = default;
};

// Removes leading and trailing spaces, as typed names often carry them.
std::string normalize_save_name(const std::string& name);

// A name is also the save's file name, so it only accepts letters, digits, spaces, '-'
// and '_', up to MaxSaveNameLength, never empty and never starting or ending with a space.
bool is_valid_save_name(const std::string& name);

// "YYYY-MM-DD HH:MM" in local time, for display.
std::string format_created_at(std::int64_t created_at);

// A new random seed for a world.
std::uint64_t random_seed();

// Current Unix time in seconds.
std::int64_t system_clock_seconds();

} // namespace game
