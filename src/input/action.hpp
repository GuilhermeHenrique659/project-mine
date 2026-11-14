#pragma once

namespace input {

// What the player means, regardless of which key was used.
enum class Action { NavigateUp, NavigateDown, NavigateLeft, NavigateRight, Confirm, Cancel, Erase,
                         MoveUp, MoveDown, MoveLeft, MoveRight };

} // namespace input
