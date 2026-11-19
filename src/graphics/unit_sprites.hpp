#pragma once

#include "graphics/anchored_sprite.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/facing.hpp"

namespace graphics {

// Unit sprites (resource/sprite/units/). character_sheet.png has one row per
// world::Direction (in enum order) and CharacterColumns columns: 0 standing, then the
// CharacterWalkFrames of the walk cycle.
inline constexpr const char* CharacterTexture = "character";
inline constexpr int CharacterWalkFrames = 6;
inline constexpr int CharacterColumns = 1 + CharacterWalkFrames;
inline constexpr float CharacterWalkFramesPerSecond = 10.0f;
inline constexpr float CharacterRunFramesPerSecond = 16.0f;
inline constexpr ui::Size CharacterFrameSize{32.0f, 48.0f};
// The pixel of a frame between the feet.
inline constexpr ui::Point CharacterAnchor{16.0f, 44.0f};

// The sprite of a character facing `facing` in `state` after `seconds` of animation:
// standing still shows the standing frame, walking and running play the walk cycle
// (running faster).
AnchoredSprite character_sprite(::world::Direction facing, ::world::CharacterState state,
                                float seconds);

} // namespace graphics
