#pragma once

#include <optional>

#include "graphics/anchored_sprite.hpp"
#include "scene/world/core/construction/building_type.hpp"

namespace graphics {

// Building sprites (resource/sprite/buildings/), drawn at the default zoom (64x32 tiles).
inline constexpr const char* HeadquartersTexture = "headquarters";
inline constexpr int HeadquartersFrames = 8; // one turn of the antenna
inline constexpr float HeadquartersFramesPerSecond = 4.0f;
inline constexpr ui::Size HeadquartersFrameSize{336.0f, 352.0f};
// The pixel of a frame that sits on the footprint centre.
inline constexpr ui::Point HeadquartersAnchor{168.0f, 264.0f};

// The sprite of a finished building of `type` after `seconds` of animation; none for the
// types that have no sprite yet (they keep their cube).
std::optional<AnchoredSprite> building_sprite(::world::BuildingType type, float seconds);

} // namespace graphics
