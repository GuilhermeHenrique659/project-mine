#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "game/save_game.hpp"
#include "persistence/save_store.hpp"
#include "support/temp_dir.hpp"

namespace {

void corrupt(const std::filesystem::path& file) {
    std::ofstream(file, std::ios::trunc) << "not a save file\n";
}

} // namespace

TEST(SaveStore, SavedGameLoadsBackUnchanged) {
    // Given
    TempDir dir("save_roundtrip");
    const persistence::SaveStore store(dir.path());
    const game::SaveGame save{"My Base", 1'700'000'000};

    // When
    const bool saved = store.save(save);
    const auto loaded = store.load("My Base");

    // Then
    EXPECT_TRUE(saved);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, save);
}

TEST(SaveStore, WorldSeedSurvivesSavingAndLoading) {
    // Given
    TempDir dir("save_seed");
    const persistence::SaveStore store(dir.path());
    const game::SaveGame save{"My Base", 1'700'000'000, 18'446'744'073'709'551'615ull};

    // When
    store.save(save);
    const auto loaded = store.load("My Base");

    // Then
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->seed, 18'446'744'073'709'551'615ull);
}

TEST(SaveStore, SaveWrittenBeforeSeedsExistedStillLoadsWithSeedZero) {
    // Given
    TempDir dir("save_legacy");
    const persistence::SaveStore store(dir.path());
    std::filesystem::create_directories(store.directory());
    std::ofstream(store.file_of("old")) << "version=1\nname=old\ncreated_at=100\n";

    // When
    const auto loaded = store.load("old");

    // Then
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->seed, 0u);
}

TEST(SaveStore, LoadingAnUnknownNameIsEmpty) {
    // Given
    TempDir dir("save_unknown");
    const persistence::SaveStore store(dir.path());

    // When
    const auto loaded = store.load("nothing here");

    // Then
    EXPECT_FALSE(loaded.has_value());
}

TEST(SaveStore, SavingAgainKeepsThePreviousVersionAsBackup) {
    // Given
    TempDir dir("save_backup");
    const persistence::SaveStore store(dir.path());
    store.save({"base", 100});

    // When
    store.save({"base", 200});

    // Then: the new save is current, the old one is recoverable from the backup
    EXPECT_EQ(store.load("base")->created_at, 200);
    corrupt(store.file_of("base"));
    const auto recovered = store.load("base");
    ASSERT_TRUE(recovered.has_value());
    EXPECT_EQ(recovered->created_at, 100);
}

TEST(SaveStore, FirstSaveCreatesNoBackup) {
    // Given
    TempDir dir("save_first");
    const persistence::SaveStore store(dir.path());

    // When
    store.save({"base", 100});

    // Then
    EXPECT_FALSE(std::filesystem::exists(store.backup_of("base")));
}

TEST(SaveStore, AThirdSaveReplacesTheExistingBackup) {
    // Given
    TempDir dir("save_backup_replace");
    const persistence::SaveStore store(dir.path());
    store.save({"base", 100});
    store.save({"base", 200});

    // When
    store.save({"base", 300});

    // Then
    EXPECT_EQ(store.load("base")->created_at, 300);
    corrupt(store.file_of("base"));
    EXPECT_EQ(store.load("base")->created_at, 200);
}

TEST(SaveStore, MissingCurrentFileLoadsFromBackup) {
    // Given
    TempDir dir("save_missing_current");
    const persistence::SaveStore store(dir.path());
    store.save({"base", 100});
    store.save({"base", 200});
    std::filesystem::remove(store.file_of("base"));

    // When
    const auto loaded = store.load("base");

    // Then
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->created_at, 100);
}

TEST(SaveStore, CorruptFileWithoutBackupIsEmpty) {
    // Given
    TempDir dir("save_corrupt");
    const persistence::SaveStore store(dir.path());
    store.save({"base", 100});
    corrupt(store.file_of("base"));

    // When
    const auto loaded = store.load("base");

    // Then
    EXPECT_FALSE(loaded.has_value());
}

TEST(SaveStore, InvalidNameIsNeverWrittenToDisk) {
    // Given
    TempDir dir("save_invalid_name");
    const persistence::SaveStore store(dir.path());

    // When
    const bool saved = store.save({"../escape", 100});

    // Then
    EXPECT_FALSE(saved);
    EXPECT_FALSE(std::filesystem::exists(dir.path() / "escape.sav"));
    EXPECT_FALSE(store.load("../escape").has_value());
}

TEST(SaveStore, ListReturnsSavesNewestFirst) {
    // Given
    TempDir dir("save_list");
    const persistence::SaveStore store(dir.path());
    store.save({"old", 100});
    store.save({"newest", 300});
    store.save({"middle", 200});

    // When
    const auto saves = store.list();

    // Then
    ASSERT_EQ(saves.size(), 3u);
    EXPECT_EQ(saves[0].name, "newest");
    EXPECT_EQ(saves[1].name, "middle");
    EXPECT_EQ(saves[2].name, "old");
}

TEST(SaveStore, ListShowsEachSaveOnceEvenWithABackup) {
    // Given
    TempDir dir("save_list_backup");
    const persistence::SaveStore store(dir.path());
    store.save({"base", 100});
    store.save({"base", 200});

    // When
    const auto saves = store.list();

    // Then
    ASSERT_EQ(saves.size(), 1u);
    EXPECT_EQ(saves[0], (game::SaveGame{"base", 200}));
}

TEST(SaveStore, ListSkipsUnreadableSavesAndForeignFiles) {
    // Given
    TempDir dir("save_list_corrupt");
    const persistence::SaveStore store(dir.path());
    store.save({"good", 100});
    store.save({"broken", 200});
    corrupt(store.file_of("broken"));
    std::ofstream(store.directory() / "notes.txt") << "hello\n";

    // When
    const auto saves = store.list();

    // Then
    ASSERT_EQ(saves.size(), 1u);
    EXPECT_EQ(saves[0].name, "good");
}

TEST(SaveStore, ListWithoutSavesDirectoryIsEmpty) {
    // Given
    TempDir dir("save_list_empty");
    const persistence::SaveStore store(dir.path());

    // When
    const auto saves = store.list();

    // Then
    EXPECT_TRUE(saves.empty());
}
