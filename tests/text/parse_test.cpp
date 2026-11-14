#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>

#include "text/parse.hpp"

TEST(Parse, ReadsTheWholeTextAsANumber) {
    // Given
    const std::string seed = "18446744073709551615";

    // When
    const auto parsed = text::parse<std::uint64_t>(seed);

    // Then
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(*parsed, std::numeric_limits<std::uint64_t>::max());
}

TEST(Parse, TextWithAnythingBesidesTheNumberIsRejected) {
    // Given
    const std::string inputs[] = {"", " 12", "12 ", "12abc", "+12", "1.5", "0x10"};

    // When / Then
    for (const std::string& input : inputs) {
        EXPECT_FALSE(text::parse<int>(input).has_value()) << '"' << input << '"';
    }
}

TEST(Parse, ANumberThatDoesNotFitIsRejected) {
    // Given
    const std::string too_big = "18446744073709551616";

    // When
    const auto parsed = text::parse<std::uint64_t>(too_big);

    // Then
    EXPECT_FALSE(parsed.has_value());
}

TEST(Parse, UnsignedTypesRejectNegativeNumbers) {
    // Given
    const std::string negative = "-1";

    // When
    const auto as_unsigned = text::parse<std::uint64_t>(negative);
    const auto as_signed = text::parse<std::int64_t>(negative);

    // Then
    EXPECT_FALSE(as_unsigned.has_value());
    ASSERT_TRUE(as_signed.has_value());
    EXPECT_EQ(*as_signed, -1);
}

TEST(Parse, FloatsReadBackExactlyWhatToCharsWrote) {
    // Given
    const float value = 0.1f + 3.0f;
    const std::string written = text::format(value);

    // When
    const auto parsed = text::parse<float>(written);

    // Then
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(*parsed, value);
}
