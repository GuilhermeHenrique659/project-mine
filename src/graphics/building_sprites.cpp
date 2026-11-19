#include "graphics/building_sprites.hpp"

#include "graphics/animation.hpp"
#include "graphics/sprite_sheet.hpp"

namespace graphics {

namespace {

// The headquarters sheet: one row of frames, the antenna turning once, in a loop.
const Animation& headquarters_animation() {
    static const Animation animation(
        SpriteSheet(HeadquartersTexture, static_cast<int>(HeadquartersFrameSize.width),
                    static_cast<int>(HeadquartersFrameSize.height), HeadquartersFrames,
                    HeadquartersFrames)
            .frames(0, HeadquartersFrames),
        HeadquartersFramesPerSecond, true);
    return animation;
}

} // namespace

std::optional<AnchoredSprite> building_sprite(::world::BuildingType type, float seconds) {
    if (type != ::world::BuildingType::Headquarters) {
        return std::nullopt;
    }
    return AnchoredSprite{*headquarters_animation().sprite_at(seconds), HeadquartersFrameSize,
                          HeadquartersAnchor};
}

} // namespace graphics
