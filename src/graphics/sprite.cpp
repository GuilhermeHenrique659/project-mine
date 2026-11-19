#include "graphics/sprite.hpp"

#include "graphics/texture_manager.hpp"

namespace graphics {

bool draw_sprite(ui::Canvas& canvas, const Sprite& sprite, ui::Rect dest,
                 ui::TextureDraw options) {
    TextureManager& textures = TextureManager::instance();
    const Texture* texture = textures.find(sprite.texture);
    options.source = sprite.source;
    if (texture == nullptr) {
        // The region belongs to the missing texture: the fallback is drawn whole.
        texture = textures.fallback();
        options.source.reset();
    }
    if (texture == nullptr) {
        return false;
    }
    canvas.draw_texture(*texture, dest, options);
    return true;
}

bool draw_loaded_sprite(ui::Canvas& canvas, const Sprite& sprite, ui::Rect dest,
                        ui::TextureDraw options) {
    const Texture* texture = TextureManager::instance().find(sprite.texture);
    if (texture == nullptr) {
        return false;
    }
    options.source = sprite.source;
    canvas.draw_texture(*texture, dest, options);
    return true;
}

bool draw_frame(ui::Canvas& canvas, const SpriteFrame& frame, ui::Rect dest,
                ui::TextureDraw options) {
    const Texture* texture = TextureManager::instance().find(frame.texture);
    if (texture == nullptr) {
        return false;
    }
    options.source = frame.source;
    canvas.draw_texture(*texture, dest, options);
    return true;
}

} // namespace graphics
