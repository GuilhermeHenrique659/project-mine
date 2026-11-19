#include "graphics/sprite_library.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <system_error>

#include "logging/logger.hpp"

namespace graphics {

namespace {

constexpr std::string_view IdleClip = "idle";
// A clip index past the sprite's clips: its first frame of the row, held.
const SpriteClip StillClip{"", 0, 1, 0.0f, true};

int wrap(int value, int count) {
    const int rest = value % count;
    return rest < 0 ? rest + count : rest;
}

} // namespace

SpriteId SpriteLibrary::add(std::string name, SpriteDefinition definition, TextureId texture) {
    const auto id = static_cast<SpriteId>(sprites_.size());
    sprites_.push_back({std::move(definition), texture});
    ids_.insert_or_assign(std::move(name), id);
    return id;
}

std::optional<SpriteId> SpriteLibrary::find(std::string_view name) const {
    const auto found = ids_.find(name);
    if (found == ids_.end()) {
        return std::nullopt;
    }
    return found->second;
}

ClipHandle SpriteLibrary::clip(SpriteId sprite, std::string_view name) const {
    const std::vector<SpriteClip>& clips = sprites_[static_cast<std::size_t>(sprite)].definition.clips;
    const auto index_of = [&](std::string_view wanted) {
        const auto found = std::find_if(clips.begin(), clips.end(),
                                        [&](const SpriteClip& clip) { return clip.name == wanted; });
        return static_cast<std::uint32_t>(found - clips.begin());
    };
    std::uint32_t index = index_of(name);
    if (index == clips.size()) {
        index = index_of(IdleClip);
    }
    return {sprite, index};
}

bool SpriteLibrary::has_clip(SpriteId sprite, std::string_view name) const {
    const std::vector<SpriteClip>& clips = sprites_[static_cast<std::size_t>(sprite)].definition.clips;
    return std::any_of(clips.begin(), clips.end(),
                       [&](const SpriteClip& clip) { return clip.name == name; });
}

int SpriteLibrary::rows(SpriteId sprite) const {
    return sprites_[static_cast<std::size_t>(sprite)].definition.rows;
}

SpriteFrame SpriteLibrary::frame(ClipHandle handle, int row, float seconds) const {
    const Entry& entry = sprites_[static_cast<std::size_t>(handle.sprite)];
    const SpriteDefinition& definition = entry.definition;
    const SpriteClip& clip =
        handle.clip < definition.clips.size() ? definition.clips[handle.clip] : StillClip;

    int step = 0;
    if (seconds > 0.0f && clip.frames_per_second > 0.0f) {
        step = static_cast<int>(std::floor(seconds * clip.frames_per_second));
        step = clip.loop ? step % clip.count : std::min(step, clip.count - 1);
    }
    const auto width = static_cast<float>(definition.frame_width);
    const auto height = static_cast<float>(definition.frame_height);
    const ui::Rect source{static_cast<float>(clip.first + step) * width,
                          static_cast<float>(wrap(row, definition.rows)) * height, width, height};
    return {entry.texture, source, {width, height}, definition.anchor};
}

SpriteLibrary load_sprite_library(TextureManager& textures, const std::filesystem::path& root) {
    SpriteLibrary library;
    std::error_code error;
    if (!std::filesystem::is_directory(root, error)) {
        logging::warning("No sprite folder at " + root.string());
        return library;
    }
    std::vector<std::filesystem::path> files;
    for (const auto& item : std::filesystem::recursive_directory_iterator(root, error)) {
        if (item.is_regular_file() && item.path().extension() == ".sprite") {
            files.push_back(item.path());
        }
    }
    // Same ids from run to run, whatever order the file system lists them in.
    std::sort(files.begin(), files.end());

    for (const std::filesystem::path& file : files) {
        std::ifstream stream(file, std::ios::binary);
        std::stringstream text;
        text << stream.rdbuf();
        std::string problem;
        auto definition = parse_sprite_definition(text.str(), &problem);
        if (!definition) {
            logging::warning("Skipping sprite " + file.string() + ": " + problem);
            continue;
        }
        std::string name = file.lexically_relative(root).replace_extension().generic_string();
        const TextureId texture = textures.register_texture(
            "sprite:" + name, (file.parent_path() / definition->texture).string());
        library.add(std::move(name), std::move(*definition), texture);
    }
    return library;
}

} // namespace graphics
