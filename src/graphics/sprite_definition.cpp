#include "graphics/sprite_definition.hpp"

#include <array>

#include "text/parse.hpp"

namespace graphics {

namespace {

constexpr std::string_view ClipPrefix = "clip.";

std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

// "first,count,fps,loop|once"
std::optional<SpriteClip> parse_clip(std::string_view name, std::string_view value) {
    std::array<std::string_view, 4> parts;
    std::size_t part = 0;
    while (part < parts.size()) {
        const auto comma = value.find(',');
        parts[part++] = trim(value.substr(0, comma));
        if (comma == std::string_view::npos) {
            break;
        }
        value.remove_prefix(comma + 1);
    }
    if (part != parts.size() || value.find(',') != std::string_view::npos) {
        return std::nullopt;
    }
    const auto first = text::parse<int>(parts[0]);
    const auto count = text::parse<int>(parts[1]);
    const auto fps = text::parse<float>(parts[2]);
    if (!first || !count || !fps || *first < 0 || *count < 1 || *fps < 0.0f) {
        return std::nullopt;
    }
    if (parts[3] != "loop" && parts[3] != "once") {
        return std::nullopt;
    }
    return SpriteClip{std::string(name), *first, *count, *fps, parts[3] == "loop"};
}

bool read_line(SpriteDefinition& definition, std::string_view key, std::string_view value,
               std::string* error) {
    const auto positive = [&](int& field) {
        const auto number = text::parse<int>(value);
        if (!number || *number < 1) {
            return fail(error, "invalid " + std::string(key) + ": " + std::string(value));
        }
        field = *number;
        return true;
    };
    const auto coordinate = [&](float& field) {
        const auto number = text::parse<float>(value);
        if (!number) {
            return fail(error, "invalid " + std::string(key) + ": " + std::string(value));
        }
        field = *number;
        return true;
    };
    if (key == "texture") {
        definition.texture = std::string(value);
        return true;
    }
    if (key == "frame_width") return positive(definition.frame_width);
    if (key == "frame_height") return positive(definition.frame_height);
    if (key == "columns") return positive(definition.columns);
    if (key == "rows") return positive(definition.rows);
    if (key == "anchor_x") return coordinate(definition.anchor.x);
    if (key == "anchor_y") return coordinate(definition.anchor.y);
    if (key.starts_with(ClipPrefix) && key.size() > ClipPrefix.size()) {
        auto clip = parse_clip(key.substr(ClipPrefix.size()), value);
        if (!clip) {
            return fail(error, "invalid " + std::string(key) + ": " + std::string(value));
        }
        definition.clips.push_back(std::move(*clip));
        return true;
    }
    return fail(error, "unknown key " + std::string(key));
}

} // namespace

std::optional<SpriteDefinition> parse_sprite_definition(std::string_view text,
                                                        std::string* error) {
    SpriteDefinition definition;
    while (!text.empty()) {
        const auto end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) {
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) {
            fail(error, "expected key=value: " + std::string(line));
            return std::nullopt;
        }
        if (!read_line(definition, trim(line.substr(0, equals)), trim(line.substr(equals + 1)),
                       error)) {
            return std::nullopt;
        }
    }
    if (definition.texture.empty()) {
        fail(error, "missing texture");
        return std::nullopt;
    }
    if (definition.frame_width < 1 || definition.frame_height < 1) {
        fail(error, "missing frame_width or frame_height");
        return std::nullopt;
    }
    for (const SpriteClip& clip : definition.clips) {
        if (clip.first + clip.count > definition.columns) {
            fail(error, "clip " + clip.name + " runs outside its row");
            return std::nullopt;
        }
    }
    return definition;
}

} // namespace graphics
