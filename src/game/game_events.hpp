#pragma once

// Business events published on the EventBus. UI code only announces what the player
// asked for; whoever owns the business rule subscribes and reacts.
namespace game {

struct NewGameRequested {};
struct LoadGameRequested {};
struct QuitRequested {};

} // namespace game
