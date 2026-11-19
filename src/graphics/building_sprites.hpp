#pragma once

#include <optional>

#include "graphics/sprite.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "ui/geometry.hpp"

namespace graphics {

// Building sprites (resource/sprite/buildings/), drawn at the default zoom (64x32 tiles).
inline constexpr const char* HeadquartersTexture = "headquarters";
inline constexpr int HeadquartersFrames = 8; // one turn of the antenna
inline constexpr float HeadquartersFramesPerSecond = 4.0f;
inline constexpr ui::Size HeadquartersFrameSize{336.0f, 352.0f};
// The pixel of a frame that sits on the footprint centre.
inline constexpr ui::Point HeadquartersAnchor{168.0f, 264.0f};

// A building picture: the sprite, its size in pixels and the pixel on the ground point.
struct BuildingSprite {
    Sprite sprite;
    ui::Size size;
    ui::Point anchor;

    // Where to draw it so the anchor lands on `ground`, at `scale` times the default zoom.
    ui::Rect dest(ui::Point ground, float scale) const;
};

// The sprite of a finished building of `type` after `seconds` of animation; none for the
// types that have no sprite yet (they keep their cube).
std::optional<BuildingSprite> building_sprite(::world::BuildingType type, float seconds);

} // namespace graphics
