#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "graphics/texture.hpp"
#include "graphics/texture_loader.hpp"

namespace graphics {

// Process-wide texture cache. The game registers a name -> file for each texture at start
// up; views and renderers ask for it by name. A texture is loaded on its first get() (it
// needs the window's GL context), kept until unload_all() and loaded only once.
class TextureManager {
public:
    static TextureManager& instance();

    // Where textures are read from. Without a loader nothing can be loaded.
    void set_loader(std::unique_ptr<TextureLoader> loader);

    // Makes `name` available. Registering a name again replaces its file.
    void register_texture(const std::string& name, const std::string& path);

    // The texture, loaded on first use. Null when the name is not registered or the file
    // cannot be loaded (a failed load is not retried). Never the fallback.
    const Texture* find(const std::string& name);

    // Like find(), but a texture that does not exist gives the fallback instead of null.
    // Null only when there is no fallback either (no loader).
    const Texture* get(const std::string& name);

    // The generic texture shown in place of a missing one, created once on first use.
    const Texture* fallback();

    // Releases every loaded texture; they load again on the next get().
    void unload_all();

    // Forgets registrations and the loader (releases loaded textures first). For tests.
    void reset();

private:
    TextureManager() = default;

    struct Entry {
        std::string path;
        std::optional<Texture> texture;
        bool failed = false;
    };

    std::unique_ptr<TextureLoader> loader_;
    std::unordered_map<std::string, Entry> entries_;
    std::optional<Texture> fallback_;
    bool fallback_failed_ = false;
};

} // namespace graphics
