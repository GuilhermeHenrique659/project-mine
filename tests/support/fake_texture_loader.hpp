#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "graphics/texture_loader.hpp"

// Hands out 32x32 textures with ids 1, 2, ... in load order and records what happened.
// "missing.png" cannot be loaded. The placeholder has id PlaceholderId, unless
// `placeholder_works` is false.
struct FakeTextureLoader : graphics::TextureLoader {
    static constexpr std::uint32_t PlaceholderId = 100;

    std::optional<graphics::Texture> load(const std::string& path) override {
        loaded.push_back(path);
        if (path == "missing.png") {
            return std::nullopt;
        }
        return graphics::Texture{static_cast<std::uint32_t>(loaded.size()), 32, 32};
    }
    std::optional<graphics::Texture> create_placeholder() override {
        ++placeholders_created;
        if (!placeholder_works) {
            return std::nullopt;
        }
        return graphics::Texture{PlaceholderId, 8, 8};
    }
    void unload(const graphics::Texture& texture) override { unloaded.push_back(texture.id); }

    bool placeholder_works = true;
    int placeholders_created = 0;
    std::vector<std::string> loaded;
    std::vector<std::uint32_t> unloaded;
};
