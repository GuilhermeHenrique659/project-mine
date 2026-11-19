#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "graphics/sprite_definition.hpp"
#include "graphics/texture_manager.hpp"
#include "ui/geometry.hpp"

namespace graphics {

// Handle of a sprite in a SpriteLibrary (an index).
enum class SpriteId : std::uint32_t {};

// A clip of a sprite, resolved once by name so drawing never looks a name up.
struct ClipHandle {
    SpriteId sprite{};
    std::uint32_t clip = 0;
};

// One frame ready to draw: the texture region, the frame size in pixels at the default
// zoom and the pixel of it that sits on the ground point.
struct SpriteFrame {
    TextureId texture{};
    ui::Rect source;
    ui::Size size;
    ui::Point anchor;

    // Where to draw it so the anchor lands on `ground`, at `scale` times the default zoom.
    ui::Rect dest(ui::Point ground, float scale) const {
        return {ground.x - anchor.x * scale, ground.y - anchor.y * scale, size.width * scale,
                size.height * scale};
    }
};

// Every sprite the game knows, by name ("buildings/headquarters"). Names are looked up
// once; frames are then computed from handles with no allocation.
class SpriteLibrary {
public:
    SpriteId add(std::string name, SpriteDefinition definition, TextureId texture);

    std::optional<SpriteId> find(std::string_view name) const;

    // The clip `name` of `sprite`; without it the "idle" clip; without that either, the
    // first frame of each row, held.
    ClipHandle clip(SpriteId sprite, std::string_view name) const;
    bool has_clip(SpriteId sprite, std::string_view name) const;

    // How many rows the sprite's sheet has (facings, variants).
    int rows(SpriteId sprite) const;

    // The frame of `clip` in sheet row `row` (wrapped into the sheet) after `seconds`:
    // looping clips start over after their last frame, the others hold it.
    SpriteFrame frame(ClipHandle clip, int row, float seconds) const;

    std::size_t size() const { return sprites_.size(); }

private:
    struct Entry {
        SpriteDefinition definition;
        TextureId texture;
    };

    std::vector<Entry> sprites_;
    std::map<std::string, SpriteId, std::less<>> ids_;
};

// Reads every `<root>/**/*.sprite` file (see parse_sprite_definition()) into a library:
// the sprite is named by its path from `root` without the extension, with '/' separators
// ("units/character"), and its texture, the file named in it next to the .sprite, is
// registered with `textures` (not loaded). A file that cannot be read is logged and skipped.
SpriteLibrary load_sprite_library(TextureManager& textures, const std::filesystem::path& root);

} // namespace graphics
