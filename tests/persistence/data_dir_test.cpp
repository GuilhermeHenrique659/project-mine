#include <gtest/gtest.h>

#include "persistence/data_dir.hpp"

TEST(DataDir, CommandLineArgumentBeatsEnvironmentVariable) {
    // Given
    const std::vector<std::string> args{"game.exe", "--data-dir", "from_args"};

    // When
    const auto dir = persistence::resolve_data_dir(args, std::string("from_env"));

    // Then
    EXPECT_EQ(dir, std::filesystem::path("from_args"));
}

TEST(DataDir, EnvironmentVariableBeatsBuildDefault) {
    // Given
    const std::vector<std::string> args{"game.exe"};

    // When
    const auto dir = persistence::resolve_data_dir(args, std::string("from_env"));

    // Then
    EXPECT_EQ(dir, std::filesystem::path("from_env"));
}

TEST(DataDir, FallsBackToBuildDefaultWhenNothingIsSet) {
    // Given
    const std::vector<std::string> args{"game.exe"};

    // When
    const auto dir = persistence::resolve_data_dir(args, std::nullopt);

    // Then
    EXPECT_EQ(dir, persistence::default_data_dir());
    EXPECT_FALSE(dir.empty());
}

TEST(DataDir, ArgumentWithoutValueIsIgnored) {
    // Given
    const std::vector<std::string> args{"game.exe", "--data-dir"};

    // When
    const auto dir = persistence::resolve_data_dir(args, std::string("from_env"));

    // Then
    EXPECT_EQ(dir, std::filesystem::path("from_env"));
}
