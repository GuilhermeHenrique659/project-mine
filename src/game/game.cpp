#include "game/game.hpp"

#include <raylib.h>

#include "game/fixed_step.hpp"
#include "game/game_events.hpp"
#include "graphics/raylib_texture_loader.hpp"
#include "graphics/texture_manager.hpp"
#include "logging/logger.hpp"
#include "scene/load_game/load_game_scene.hpp"
#include "scene/menu/title_scene.hpp"
#include "scene/new_game/new_game_scene.hpp"
#include "scene/settings/settings_scene.hpp"
#include "scene/world/world_scene.hpp"

namespace game {

namespace {
constexpr int TargetFps = 60;
constexpr const char* WindowTitle = "Project Mine";

// Textures the game knows about (none yet: the player is drawn as a cube). Files load
// lazily on first use; a missing name gives the fallback texture.
void register_textures() {
    auto& textures = graphics::TextureManager::instance();
    textures.set_loader(std::make_unique<graphics::RaylibTextureLoader>());
}
} // namespace

Game::Game(const std::filesystem::path& data_dir, Clock clock, SeedSource seed_source)
    : display_settings_(persistence::SettingsStore(data_dir).load().value_or(DisplaySettings{})),
      settings_persistence_(bus_, persistence::SettingsStore(data_dir)),
      save_store_(data_dir),
      clock_(std::move(clock)),
      seed_source_(std::move(seed_source)) {
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
    subscriptions_.push_back(event::subscribe_scoped<NewGameRequested>(
        bus_, [this](const NewGameRequested&) {
            change_scene(scene::new_game::make_new_game_scene(*this));
        }));
    subscriptions_.push_back(event::subscribe_scoped<LoadGameRequested>(
        bus_, [this](const LoadGameRequested&) {
            change_scene(scene::load_game::make_load_game_scene(*this));
        }));
    subscriptions_.push_back(event::subscribe_scoped<CreateGameRequested>(
        bus_, [this](const CreateGameRequested& event) { create_game(event.name); }));
    subscriptions_.push_back(event::subscribe_scoped<LoadSaveRequested>(
        bus_, [this](const LoadSaveRequested& event) { load_game(event.name); }));
    subscriptions_.push_back(event::subscribe_scoped<SaveGameRequested>(
        bus_, [this](const SaveGameRequested& event) { save_store_.save(event.save); }));
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
    // Raylib closes the window on Esc by default; Esc is the Pause/Cancel action here.
    SetExitKey(KEY_NULL);
    SetTargetFPS(TargetFps);
    register_textures();
    running_ = true;
    change_scene(scene::menu::make_title_scene(*this));
}

void Game::run() {
    while (running_ && !WindowShouldClose()) {
        update(clamp_frame_time(GetFrameTime()));
        draw();
    }
    // Textures live on the GPU, so they go before the window.
    graphics::TextureManager::instance().reset();
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

// A new game exists on disk before it is played. An existing save with the same name is
// overwritten (the store keeps it as the backup).
void Game::create_game(const std::string& name) {
    if (!is_valid_save_name(name)) {
        logging::warning("Cannot create a game named \"" + name + "\"");
        return;
    }
    SaveGame save{name, clock_(), seed_source_()};
    if (!save_store_.save(save)) {
        return;
    }
    change_scene(scene::world::make_world_scene(*this, std::move(save)));
}

void Game::load_game(const std::string& name) {
    auto save = save_store_.load(name);
    if (!save) {
        return;
    }
    change_scene(scene::world::make_world_scene(*this, std::move(*save)));
}

void Game::apply_pending_scene() {
    if (!pending_scene_) {
        return;
    }
    active_scene_ = std::move(pending_scene_);
}

} // namespace game
