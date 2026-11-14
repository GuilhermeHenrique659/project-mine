#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "event/event_bus.hpp"
#include "game/display_settings.hpp"
#include "persistence/settings_persistence.hpp"
#include "scene/scene.hpp"

namespace game {

// Owns the window lifecycle and manages the active scene.
class Game {
public:
    explicit Game(const std::filesystem::path& data_dir);

    // Opens the window and loads the first scene. Call once, before the loop.
    void start_up();

    // Runs the main loop until the window is closed or quit() is called.
    void run();

    // Advances one frame: applies a pending scene change, then updates the active scene.
    void update(float delta_time);

    // Draws the active scene.
    void draw() const;

    // Replaces the active scene at the start of the next update, so the
    // scene requesting the change is never destroyed while it is running.
    void change_scene(std::unique_ptr<scene::Scene> next_scene);

    // Channel shared by the layers of every scene.
    event::EventBus& event_bus() { return bus_; }

    // What the player last chose in the settings screen (applied to the window).
    const DisplaySettings& display_settings() const { return display_settings_; }

    void quit();
    bool is_running() const;
    const scene::Scene* active_scene() const;

private:
    void apply_pending_scene();
    void apply_display_settings() const;

    // Declared first so it outlives the scenes that subscribe to it.
    event::EventBus bus_;
    std::vector<event::ScopedSubscription> subscriptions_;
    std::unique_ptr<scene::Scene> active_scene_;
    std::unique_ptr<scene::Scene> pending_scene_;
    DisplaySettings display_settings_;
    // Declared after bus_ so it unsubscribes before the bus is destroyed.
    persistence::SettingsPersistence settings_persistence_;
    bool running_ = false;
};

} // namespace game
