#include <gtest/gtest.h>

#include <string>

#include "game/save_game.hpp"

TEST(SaveName, LettersDigitsSpacesDashesAndUnderscoresAreValid) {
    // Given
    const std::string name = "My Base_2 - north";

    // When
    const bool valid = game::is_valid_save_name(name);

    // Then
    EXPECT_TRUE(valid);
}

TEST(SaveName, EmptyNameIsInvalid) {
    // Given
    const std::string name;

    // When
    const bool valid = game::is_valid_save_name(name);

    // Then
    EXPECT_FALSE(valid);
}

TEST(SaveName, CharactersThatBreakFileNamesAreInvalid) {
    // Given
    const std::string names[] = {"a/b", "a\\b", "a:b", "a*b", "a?b", "..", "a.b", "a\"b"};

    // When / Then
    for (const std::string& name : names) {
        EXPECT_FALSE(game::is_valid_save_name(name)) << name;
    }
}

TEST(SaveName, NameAtTheLengthLimitIsValidAndOneMoreIsNot) {
    // Given
    const std::string at_limit(game::MaxSaveNameLength, 'a');
    const std::string over_limit(game::MaxSaveNameLength + 1, 'a');

    // When
    const bool at_limit_valid = game::is_valid_save_name(at_limit);
    const bool over_limit_valid = game::is_valid_save_name(over_limit);

    // Then
    EXPECT_TRUE(at_limit_valid);
    EXPECT_FALSE(over_limit_valid);
}

TEST(SaveName, SurroundingSpacesAreInvalidUntilNormalized) {
    // Given
    const std::string typed = "  base  ";

    // When
    const std::string normalized = game::normalize_save_name(typed);

    // Then
    EXPECT_FALSE(game::is_valid_save_name(typed));
    EXPECT_EQ(normalized, "base");
    EXPECT_TRUE(game::is_valid_save_name(normalized));
}

TEST(SaveName, OnlySpacesNormalizeToAnInvalidEmptyName) {
    // Given
    const std::string typed = "   ";

    // When
    const std::string normalized = game::normalize_save_name(typed);

    // Then
    EXPECT_FALSE(game::is_valid_save_name(normalized));
}

TEST(SaveGameDate, CreationTimeIsShownAsDateAndMinutes) {
    // Given: a moment far from any day boundary in every time zone is not needed,
    // only the shape is checked because the text is in local time.
    const std::int64_t created_at = 1'700'000'000;

    // When
    const std::string text = game::format_created_at(created_at);

    // Then: YYYY-MM-DD HH:MM
    ASSERT_EQ(text.size(), 16u);
    EXPECT_EQ(text.substr(0, 3), "202");
    EXPECT_EQ(text[4], '-');
    EXPECT_EQ(text[10], ' ');
    EXPECT_EQ(text[13], ':');
}
