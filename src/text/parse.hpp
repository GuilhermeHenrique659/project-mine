#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace text {

// Reads the whole text as a number of type T (any integer or floating-point type); empty
// when the text is empty, has anything besides the number (spaces, a '+', trailing
// characters), or the number does not fit in T. Unsigned types reject a '-'.
// Locale-independent, for reading files the game wrote.
template <typename T>
    requires std::is_arithmetic_v<T> && (!std::is_same_v<T, bool>)
std::optional<T> parse(std::string_view text) {
    T value{};
    const char* const end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, value);
    if (error != std::errc{} || stop != end || text.empty()) {
        return std::nullopt;
    }
    return value;
}

// The shortest text that parse<T>() reads back as exactly the same value.
template <typename T>
    requires std::is_arithmetic_v<T> && (!std::is_same_v<T, bool>)
std::string format(T value) {
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

} // namespace text
