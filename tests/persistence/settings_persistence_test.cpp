#include <gtest/gtest.h>

#include <memory>

#include "game/game_events.hpp"
#include "persistence/settings_persistence.hpp"
#include "support/temp_dir.hpp"

TEST(SettingsPersistence, ChangedSettingsAreWrittenToDisk) {
    // Given
    TempDir dir("persist_write");
    event::EventBus bus;
    persistence::SettingsPersistence saver(bus, persistence::SettingsStore(dir.path()));
    const game::DisplaySettings chosen{game::resolution_presets()[1], true};

    // When
    bus.publish(game::DisplaySettingsChanged{chosen});

    // Then
    const auto loaded = persistence::SettingsStore(dir.path()).load();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, chosen);
}

TEST(SettingsPersistence, StopsSavingOnceDestroyed) {
    // Given
    TempDir dir("persist_destroyed");
    event::EventBus bus;
    auto saver = std::make_unique<persistence::SettingsPersistence>(
        bus, persistence::SettingsStore(dir.path()));
    saver.reset();

    // When
    bus.publish(game::DisplaySettingsChanged{game::DisplaySettings{{1920, 1080}, true}});

    // Then
    EXPECT_FALSE(persistence::SettingsStore(dir.path()).load().has_value());
}
