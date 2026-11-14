#pragma once

namespace world {

// What a character is doing right now.
enum class CharacterState { Idle, Moving, Running };

// Marks an entity as a character (a unit that walks on its own legs) and holds its
// state. Systems that only apply to characters require it.
struct Character {
    CharacterState state = CharacterState::Idle;
};

// Name of a state for the player to read ("Idle", "Moving").
constexpr const char* to_string(CharacterState state) {
    switch (state) {
        case CharacterState::Moving: return "Moving";
        case CharacterState::Running: return "Running";
        case CharacterState::Idle: break;
    }
    return "Idle";
}

} // namespace world
