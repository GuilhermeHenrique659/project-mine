#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "persistence/env_file.hpp"
#include "temp_dir.hpp"

namespace {

std::filesystem::path write_env(const TempDir& dir, const std::string& content) {
    std::filesystem::create_directories(dir.path());
    const auto file = dir.path() / ".env";
    std::ofstream(file, std::ios::binary) << content;
    return file;
}

} // namespace

TEST(EnvFile, ReadsNameValuePairsSkippingCommentsAndBlankLines) {
    // Given
    TempDir dir("env_parse");
    const auto file = write_env(dir, "# comment\n\nPROJECT_MINE_DATA_DIR=resource\nOTHER = two words \nnot a pair\n");

    // When
    const auto values = persistence::read_env_file(file);

    // Then
    EXPECT_EQ(values.size(), 2u);
    EXPECT_EQ(values.at("PROJECT_MINE_DATA_DIR"), "resource");
    EXPECT_EQ(values.at("OTHER"), "two words");
}

TEST(EnvFile, WindowsLineEndingsDoNotLeakIntoValues) {
    // Given
    TempDir dir("env_crlf");
    const auto file = write_env(dir, "PROJECT_MINE_DATA_DIR=resource\r\n");

    // When
    const auto values = persistence::read_env_file(file);

    // Then
    EXPECT_EQ(values.at("PROJECT_MINE_DATA_DIR"), "resource");
}

TEST(EnvFile, MissingFileGivesNoValues) {
    // Given
    TempDir dir("env_missing");

    // When
    const auto values = persistence::read_env_file(dir.path() / ".env");

    // Then
    EXPECT_TRUE(values.empty());
}

TEST(EnvFile, RealEnvironmentVariableBeatsTheFile) {
    // Given
    const persistence::EnvValues file{{"PROJECT_MINE_DATA_DIR", "from_file"}};

    // When
    const auto value = persistence::env_value("PROJECT_MINE_DATA_DIR", std::string("from_process"), file);

    // Then
    EXPECT_EQ(value, "from_process");
}

TEST(EnvFile, FileValueIsUsedWhenTheVariableIsNotSet) {
    // Given
    const persistence::EnvValues file{{"PROJECT_MINE_DATA_DIR", "from_file"}};

    // When
    const auto value = persistence::env_value("PROJECT_MINE_DATA_DIR", std::nullopt, file);

    // Then
    EXPECT_EQ(value, "from_file");
}

TEST(EnvFile, UnsetEverywhereGivesNothing) {
    // Given
    const persistence::EnvValues file;

    // When
    const auto value = persistence::env_value("PROJECT_MINE_DATA_DIR", std::nullopt, file);

    // Then
    EXPECT_FALSE(value.has_value());
}
