#include <gtest/gtest.h>

#include <optional>

#include "event/event_bus.hpp"
#include "support/fake_canvas.hpp"
#include "game/display_settings.hpp"
#include "game/game_events.hpp"
#include "scene/settings/settings_view.hpp"

namespace {

constexpr float ScreenWidth = 1280.0f;

// Records the business events the settings screen publishes.
struct EventRecorder {
    explicit EventRecorder(event::EventBus& bus) {
        subscriptions.push_back(event::subscribe_scoped<game::DisplaySettingsChanged>(
            bus, [this](const game::DisplaySettingsChanged& e) { last_change = e.settings; }));
        subscriptions.push_back(event::subscribe_scoped<game::BackToMenuRequested>(
            bus, [this](const game::BackToMenuRequested&) { ++back_requests; }));
    }

    std::optional<game::DisplaySettings> last_change;
    int back_requests = 0;
    std::vector<event::ScopedSubscription> subscriptions;
};

ui::Point center_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(SettingsView, StartsShowingTheCurrentSettings) {
    // Given
    event::EventBus bus;
    const auto& presets = game::resolution_presets();
    const game::DisplaySettings current{presets[2], true};

    // When
    scene::settings::SettingsView view(bus, current);

    // Then
    EXPECT_EQ(view.resolution_dropdown().selected(), 2u);
    EXPECT_TRUE(view.fullscreen_checkbox().checked());
}

TEST(SettingsView, ResolutionDropdownListsEveryPreset) {
    // Given
    event::EventBus bus;

    // When
    scene::settings::SettingsView view(bus, game::DisplaySettings{});

    // Then
    EXPECT_EQ(view.resolution_dropdown().option_count(), game::resolution_presets().size());
}

TEST(SettingsView, ChoosingAResolutionPublishesItKeepingFullscreen) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    const auto& presets = game::resolution_presets();
    scene::settings::SettingsView view(bus, game::DisplaySettings{presets[0], true});
    view.layout(canvas, ScreenWidth);
    view.handle_pointer(center_of(view.resolution_dropdown().bounds()), true);

    // When
    view.handle_pointer(center_of(view.resolution_dropdown().option_bounds(3)), true);

    // Then
    ASSERT_TRUE(recorder.last_change.has_value());
    EXPECT_EQ(recorder.last_change->resolution, presets[3]);
    EXPECT_TRUE(recorder.last_change->fullscreen);
}

TEST(SettingsView, TogglingFullscreenPublishesItKeepingResolution) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    const auto& presets = game::resolution_presets();
    scene::settings::SettingsView view(bus, game::DisplaySettings{presets[1], false});
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view.fullscreen_checkbox().bounds()), true);

    // Then
    ASSERT_TRUE(recorder.last_change.has_value());
    EXPECT_TRUE(recorder.last_change->fullscreen);
    EXPECT_EQ(recorder.last_change->resolution, presets[1]);
}

TEST(SettingsView, BackButtonPublishesBackToMenuRequested) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    FakeCanvas canvas;
    scene::settings::SettingsView view(bus, game::DisplaySettings{});
    view.layout(canvas, ScreenWidth);

    // When
    view.handle_pointer(center_of(view.back_button().bounds()), true);

    // Then
    EXPECT_EQ(recorder.back_requests, 1);
    EXPECT_FALSE(recorder.last_change.has_value());
}

TEST(SettingsView, KeyboardCanReachEveryControl) {
    // Given
    event::EventBus bus;
    EventRecorder recorder(bus);
    scene::settings::SettingsView view(bus, game::DisplaySettings{});

    // When: dropdown -> checkbox -> back button
    view.focus_group().focus_next();
    view.focus_group().focus_next();
    view.focus_group().activate_focused();

    // Then
    EXPECT_EQ(recorder.back_requests, 1);
}
