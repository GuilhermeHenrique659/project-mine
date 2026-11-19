#include "graphics/unit_sprites.hpp"

#include <array>

#include "graphics/animation.hpp"
#include "graphics/sprite_sheet.hpp"

namespace graphics {

namespace {

const SpriteSheet& character_sheet() {
    static const SpriteSheet sheet(CharacterTexture, static_cast<int>(CharacterFrameSize.width),
                                   static_cast<int>(CharacterFrameSize.height), CharacterColumns,
                                   CharacterColumns * ::world::DirectionCount);
    return sheet;
}

// The walk cycle of every facing (one row each) at `frames_per_second`, in a loop.
std::array<Animation, ::world::DirectionCount> walk_cycles(float frames_per_second) {
    const auto cycle = [frames_per_second](int row) {
        return Animation(character_sheet().frames(row * CharacterColumns + 1, CharacterWalkFrames),
                         frames_per_second, true);
    };
    return {cycle(0), cycle(1), cycle(2), cycle(3), cycle(4), cycle(5), cycle(6), cycle(7)};
}

} // namespace

AnchoredSprite character_sprite(::world::Direction facing, ::world::CharacterState state,
                                float seconds) {
    static const auto walking = walk_cycles(CharacterWalkFramesPerSecond);
    static const auto running = walk_cycles(CharacterRunFramesPerSecond);
    const int row = static_cast<int>(facing);

    Sprite sprite = *character_sheet().frame(row * CharacterColumns);
    if (state == ::world::CharacterState::Moving) {
        sprite = *walking[row].sprite_at(seconds);
    } else if (state == ::world::CharacterState::Running) {
        sprite = *running[row].sprite_at(seconds);
    }
    return {sprite, CharacterFrameSize, CharacterAnchor};
}

} // namespace graphics
