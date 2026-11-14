#pragma once

#include "game/display_settings.hpp"

// Business events published on the EventBus. UI code only announces what the player
// asked for; whoever owns the business rule subscribes and reacts.
namespace game {

struct NewGameRequested {};
struct LoadGameRequested {};
struct SettingsRequested {};
struct BackToMenuRequested {};
struct QuitRequested {};

// The player changed the resolution or fullscreen option; carries the full new state.
struct DisplaySettingsChanged {
    DisplaySettings settings;
};

} // namespace game
