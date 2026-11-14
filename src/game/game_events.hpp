#pragma once

#include <string>

#include "game/display_settings.hpp"
#include "game/save_game.hpp"

// Business events published on the EventBus. UI code only announces what the player
// asked for; whoever owns the business rule subscribes and reacts.
namespace game {

// The player wants to start / continue a game: opens the name prompt / the save list.
struct NewGameRequested {};
struct LoadGameRequested {};

// The player confirmed the name of a new game (already normalized by the view).
struct CreateGameRequested {
    std::string name;
};

// The player picked an existing save to continue.
struct LoadSaveRequested {
    std::string name;
};

// The player asked to write the running game to disk; carries the full state to save.
struct SaveGameRequested {
    SaveGame save;
};

struct SettingsRequested {};
struct BackToMenuRequested {};
struct QuitRequested {};

// The player changed the resolution or fullscreen option; carries the full new state.
struct DisplaySettingsChanged {
    DisplaySettings settings;
};

} // namespace game
