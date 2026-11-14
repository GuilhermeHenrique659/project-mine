#include "game/game.hpp"

#include <raylib.h>

#include "game/game_events.hpp"
#include "scene/menu/title_scene.hpp"
#include "scene/settings/settings_scene.hpp"

namespace game {

namespace {
constexpr int TargetFps = 60;
constexpr const char* WindowTitle = "Project Mine";
} // namespace

Game::Game(const std::filesystem::path& data_dir)
    : display_settings_(persistence::SettingsStore(data_dir).load().value_or(DisplaySettings{})),
      settings_persistence_(bus_, persistence::SettingsStore(data_dir)) {
    subscriptions_.push_back(event::subscribe_scoped<QuitRequested>(
        bus_, [this](const QuitRequested&) { quit(); }));
    subscriptions_.push_back(event::subscribe_scoped<SettingsRequested>(
        bus_, [this](const SettingsRequested&) {
            change_scene(scene::settings::make_settings_scene(*this));
        }));
    subscriptions_.push_back(event::subscribe_scoped<BackToMenuRequested>(
        bus_, [this](const BackToMenuRequested&) {
            change_scene(scene::menu::make_title_scene(*this));
        }));
    subscriptions_.push_back(event::subscribe_scoped<DisplaySettingsChanged>(
        bus_, [this](const DisplaySettingsChanged& event) {
            display_settings_ = event.settings;
            apply_display_settings();
        }));
}

void Game::start_up() {
    InitWindow(display_settings_.resolution.width, display_settings_.resolution.height,
               WindowTitle);
    apply_display_settings();
    SetTargetFPS(TargetFps);
    running_ = true;
    change_scene(scene::menu::make_title_scene(*this));
}

void Game::run() {
    while (running_ && !WindowShouldClose()) {
        update(GetFrameTime());
        draw();
    }
    CloseWindow();
}

void Game::update(float delta_time) {
    apply_pending_scene();
    if (!active_scene_) {
        return;
    }
    active_scene_->update(delta_time);
}

void Game::draw() const {
    BeginDrawing();
    ClearBackground(BLACK);
    if (active_scene_) {
        active_scene_->draw();
    }
    EndDrawing();
}

void Game::change_scene(std::unique_ptr<scene::Scene> next_scene) {
    pending_scene_ = std::move(next_scene);
}

void Game::quit() { running_ = false; }

bool Game::is_running() const { return running_; }

const scene::Scene* Game::active_scene() const { return active_scene_.get(); }

// Thin raylib shell: the decision (what the player chose) lives in display_settings_.
// Without a window (tests) there is nothing to apply.
void Game::apply_display_settings() const {
    if (!IsWindowReady()) {
        return;
    }
    if (display_settings_.fullscreen != IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    if (display_settings_.fullscreen) {
        return;
    }
    SetWindowSize(display_settings_.resolution.width, display_settings_.resolution.height);
}

void Game::apply_pending_scene() {
    if (!pending_scene_) {
        return;
    }
    active_scene_ = std::move(pending_scene_);
}

} // namespace game
